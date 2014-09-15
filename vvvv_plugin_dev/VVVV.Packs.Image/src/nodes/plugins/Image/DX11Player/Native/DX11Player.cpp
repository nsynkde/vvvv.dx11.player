// Native.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "DX11Player.h"
#include "tinydir.h"
#include <comdef.h>
#include <sstream>
#include <iostream>
#include <math.h>
#include "Offset.h"

static const int HEADER_SIZE_BYTES = 128;
static const int TEXTURE_WIDTH = 3840;
static const int TEXTURE_HEIGHT = 2160;
static const int TEXTURE_SIZE_BYTES = TEXTURE_WIDTH * TEXTURE_HEIGHT * 4;
static const int BUFFER_SIZE_BYTES = TEXTURE_SIZE_BYTES + HEADER_SIZE_BYTES;
static const int RING_BUFFER_SIZE = 8;
static const int OUTPUT_BUFFER_SIZE = 4;
static const int FPS = 60;

DX11Player::DX11Player(ID3D11Device * device, const std::string & directory)
	:m_Device(device)
	,m_Context(nullptr)
	,m_CopyTextureIn(RING_BUFFER_SIZE)
	,m_CopyTextureOut(RING_BUFFER_SIZE)
	,m_ShaderResourceView(RING_BUFFER_SIZE)
	,m_UAVs(RING_BUFFER_SIZE)
	,m_OutputTextures(RING_BUFFER_SIZE)
	,m_UploadBuffers(RING_BUFFER_SIZE)
	,m_Overlaps(RING_BUFFER_SIZE)
	,m_WaitEvents(RING_BUFFER_SIZE)
	,m_FileHandles(RING_BUFFER_SIZE)
	,m_MappedBuffers(RING_BUFFER_SIZE)
	,m_UploaderThreadRunning(false)
	,m_WaiterThreadRunning(false)
	,m_CurrentFrame(0)
	,m_NewFrame(false)
	,m_CurrentOutFront(RING_BUFFER_SIZE/2)
	,m_CurrentOutBack(0)
{
	HRESULT hr;

	if(device==nullptr){
		OutputDebugString( L"creating device" );
		D3D_FEATURE_LEVEL level;
		hr = D3D11CreateDevice(nullptr,
			D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			0,
			nullptr,
			0,
			D3D11_SDK_VERSION,
			&m_Device,&level,
			&m_Context);

		if(FAILED(hr)){
			auto lpBuf = new char[1024];
			ZeroMemory(lpBuf,sizeof(lpBuf));
			FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL, hr, 0, lpBuf, 0, NULL);
			throw std::exception((std::string("Coudln't create device: ") + lpBuf).c_str());
		}
	}else{
		device->GetImmediateContext(&m_Context);
	}

	OutputDebugString( L"zeroing read back data" );
	for(auto & map: m_MappedBuffers){
		ZeroMemory(&map,sizeof(D3D11_MAPPED_SUBRESOURCE));
	}

	OutputDebugString( L"creating render textures" );
	D3D11_TEXTURE2D_DESC textureDescription;
    textureDescription.MipLevels = 1;
    textureDescription.ArraySize = 1;
	textureDescription.SampleDesc.Count = 1;
	textureDescription.SampleDesc.Quality = 0;
	textureDescription.Usage = D3D11_USAGE_DEFAULT;
	textureDescription.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    textureDescription.CPUAccessFlags = 0;
	if(device==nullptr){
		textureDescription.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
	}else{
		textureDescription.MiscFlags = 0;
	}
    textureDescription.Width = TEXTURE_WIDTH;
    textureDescription.Height = TEXTURE_HEIGHT;
	textureDescription.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	for(size_t i=0;i<m_OutputTextures.size();i++){
		hr = m_Device->CreateTexture2D(&textureDescription,nullptr,&m_OutputTextures[i]);
		if(FAILED(hr)){
			throw std::exception("Coudln't create render texture");
		} 

		if(!device){
			OutputDebugString( L"getting shared texture handle" );
			IDXGIResource* pTempResource(NULL);
			HANDLE sharedHandle;
			hr = m_OutputTextures[i]->QueryInterface( __uuidof(IDXGIResource), (void**)&pTempResource );
			if(FAILED(hr)){
				throw std::exception("Coudln't query interface");
			}
			hr = pTempResource->GetSharedHandle(&sharedHandle);
			if(FAILED(hr)){
				throw std::exception("Coudln't get shared handle");
			}
			m_SharedHandles[m_OutputTextures[i]] = sharedHandle;
			pTempResource->Release();
		}
	}
	
	OutputDebugString( L"creating render textures" );
	D3D11_TEXTURE2D_DESC textureDescriptionCopy;
    textureDescriptionCopy.MipLevels = 1;
    textureDescriptionCopy.ArraySize = 1;
	textureDescriptionCopy.SampleDesc.Count = 1;
	textureDescriptionCopy.SampleDesc.Quality = 0;
	textureDescriptionCopy.Usage = D3D11_USAGE_DEFAULT;
	textureDescriptionCopy.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    textureDescriptionCopy.CPUAccessFlags = 0;
	textureDescriptionCopy.MiscFlags = 0;
    textureDescriptionCopy.Width = TEXTURE_WIDTH;
    textureDescriptionCopy.Height = TEXTURE_HEIGHT;
	textureDescriptionCopy.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	for(size_t i=0;i<m_CopyTextureOut.size();i++){
		hr = m_Device->CreateTexture2D(&textureDescriptionCopy,nullptr,&m_CopyTextureIn[i]);
	}
	textureDescriptionCopy.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
	for(size_t i=0;i<m_CopyTextureOut.size();i++){
		hr = m_Device->CreateTexture2D(&textureDescriptionCopy,nullptr,&m_CopyTextureOut[i]);
	}
	

	for(size_t i = 0; i < m_UploadBuffers.size(); i++){
		OutputDebugString( L"creating staging textures" );
		hr = CreateStagingTexture(TEXTURE_WIDTH,TEXTURE_HEIGHT,DXGI_FORMAT_R8G8B8A8_UNORM,&m_UploadBuffers[i]);
		if(FAILED(hr)){
			_com_error error(hr);
			auto msg = error.ErrorMessage();

			auto lpBuf = new char[1024];
			ZeroMemory(lpBuf,sizeof(lpBuf));
			FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), lpBuf, 0, NULL);
			throw std::exception((std::string("Coudln't create staging texture") + lpBuf).c_str());
		}
	}

	ID3D11ComputeShader * shader;
	m_Device->CreateComputeShader(Offset,sizeof(Offset),nullptr,&shader);
	if(FAILED(hr)){
		throw std::exception("Coudln't create compute shader");
	}
	m_Context->CSSetShader(shader,nullptr,0);
	
	for(size_t i=0;i<m_CopyTextureOut.size();i++){
		hr = m_Device->CreateShaderResourceView(m_CopyTextureIn[i],nullptr,&m_ShaderResourceView[i]);
		if(FAILED(hr)){
			throw std::exception("Coudln't create shader resource view");
		}
	}
	m_Context->CSSetShaderResources(0,1,&m_ShaderResourceView[0]);

	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	uavDesc.Texture2D.MipSlice = 0;
	uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	for(size_t i=0;i<m_UAVs.size();i++){
		m_Device->CreateUnorderedAccessView(m_CopyTextureOut[i],&uavDesc,&m_UAVs[i]);
	}
	m_Context->CSSetUnorderedAccessViews(0,1,&m_UAVs[0],0);

	_declspec(align(16))
	struct PS_CONSTANT_BUFFER
	{
		int InputWidth;
		int Offset;
	};

	PS_CONSTANT_BUFFER constantData;
	constantData.InputWidth = TEXTURE_WIDTH;
	constantData.Offset = HEADER_SIZE_BYTES/4;

	D3D11_BUFFER_DESC shaderVarsDescription;
	shaderVarsDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	shaderVarsDescription.ByteWidth = sizeof(PS_CONSTANT_BUFFER);
	shaderVarsDescription.CPUAccessFlags = 0;
	shaderVarsDescription.MiscFlags = 0;
	shaderVarsDescription.StructureByteStride = 0;
	shaderVarsDescription.Usage = D3D11_USAGE_DEFAULT;
		
	D3D11_SUBRESOURCE_DATA bufferData;
	bufferData.pSysMem = &constantData;
	bufferData.SysMemPitch = 0;
	bufferData.SysMemSlicePitch = 0;

	ID3D11Buffer * varsBuffer;
	hr = m_Device->CreateBuffer(&shaderVarsDescription,&bufferData,&varsBuffer);
	if(FAILED(hr)){
		throw std::exception("Coudln't create constant buffer");
	}
	OutputDebugString( L"Created constants buffer" );
        
	m_Context->CSSetConstantBuffers(0,1,&varsBuffer);
	OutputDebugString( L"Set constants buffer" );
	varsBuffer->Release();

	
	tinydir_dir dir;
	tinydir_open_sorted(&dir, directory.c_str());
	std::stringstream str;
	str << "reading folder " <<  directory <<  " with " << dir.n_files << " files";
	OutputDebugStringA( str.str().c_str());
	for (int i = 0; i < dir.n_files; i++)
	{

		tinydir_file file;
		tinydir_readfile_n(&dir, &file, i);

		if (!file.is_dir)
		{
			m_ImageFiles.push_back(directory+"\\"+std::string(file.name));
		}
	}

	tinydir_close(&dir); 
	

	for(int i=0;i<RING_BUFFER_SIZE;i++){
		m_Context->Map(m_UploadBuffers[i],0,D3D11_MAP_WRITE,0,&m_MappedBuffers[i]);
		m_ReadyToUpload.send(i);
		m_WaitEvents[i] = CreateEventW(0,false,false,0);
		ZeroMemory(&m_Overlaps[i],sizeof(OVERLAPPED));
	}

	
	OutputDebugString( L"creating upload thread" );
	m_UploaderThreadRunning = true;
	auto running = &m_UploaderThreadRunning;
	auto currentFrame = &m_CurrentFrame;
	auto imageFiles = &m_ImageFiles;
	auto readyToUpload = &m_ReadyToUpload;
	auto readyToWait = &m_ReadyToWait;
	auto readyToRender = &m_ReadyToRender;
	auto mappedBuffers = &m_MappedBuffers;
	auto overlaps = &m_Overlaps;
	auto waitEvents = &m_WaitEvents;
	auto fileHandlers = &m_FileHandles;
	m_UploaderThread = std::thread([running,currentFrame,imageFiles,readyToUpload,readyToWait,readyToRender,mappedBuffers,overlaps,waitEvents,fileHandlers]{
		while(*running){
			int nextBuffer;
			readyToUpload->recv(nextBuffer);
			auto ptr = (char*)mappedBuffers->at(nextBuffer).pData;
			if(ptr){
				auto file = CreateFileA(imageFiles->at(*currentFrame).c_str(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED,NULL);
				if (file != INVALID_HANDLE_VALUE) {
					ZeroMemory(&overlaps->at(nextBuffer),sizeof(OVERLAPPED));
					overlaps->at(nextBuffer).hEvent = waitEvents->at(nextBuffer);
					fileHandlers->at(nextBuffer) = file;
					DWORD bytesRead=0;
					DWORD totalBytesRead=0;
					DWORD totalToRead = TEXTURE_SIZE_BYTES;
					//SetFilePointer(file,HEADER_SIZE_BYTES,nullptr,FILE_BEGIN);
					//while(totalBytesRead<totalToRead){
						ReadFile(file,ptr+totalBytesRead,totalToRead - totalBytesRead,&bytesRead,&overlaps->at(nextBuffer));
						//totalBytesRead += bytesRead;
					//}
					readyToWait->send(nextBuffer);
				}

				(*currentFrame)+=1;
				(*currentFrame)%=imageFiles->size();
			}
		} 
	});
	
	OutputDebugString( L"creating waiter thread" );
	m_WaiterThreadRunning = true;
	running = &m_WaiterThreadRunning;
	auto context = m_Context;
	auto copyTextureIn = &m_CopyTextureIn;
	auto copyTextureOut = &m_CopyTextureOut;
	auto uploadBuffers = &m_UploadBuffers;
	auto uavs = &m_UAVs;
	auto outputTextures = &m_OutputTextures;
	auto currentBack = &m_CurrentOutBack;
	m_WaiterThread = std::thread([running,readyToWait,readyToRender,waitEvents,fileHandlers,context,copyTextureIn,copyTextureOut,uploadBuffers,uavs,mappedBuffers,outputTextures,currentBack]{
		while(*running){
			int nextBuffer;
			readyToWait->recv(nextBuffer);
			auto waitEvent = waitEvents->at(nextBuffer);
			WaitForSingleObject(waitEvent,INFINITE);
			CloseHandle(fileHandlers->at(nextBuffer));

			
			/*context->Unmap(uploadBuffers->at(nextBuffer), 0);
			context->CopyResource(copyTextureIn->at(0),uploadBuffers->at(nextBuffer));
			context->Dispatch(TEXTURE_WIDTH/32,TEXTURE_HEIGHT/30,1);
			context->CopyResource(outputTextures->at(nextBuffer),copyTextureOut->at(0));
			context->Map(uploadBuffers->at(nextBuffer),0,D3D11_MAP_WRITE,0,&mappedBuffers->at(nextBuffer));*/
			readyToRender->send(nextBuffer);
		}
	});

	m_LastFrame = HighResClock::now();
}

HRESULT DX11Player::CreateStagingTexture(int Width, int Height, DXGI_FORMAT Format, ID3D11Texture2D ** texture){
	D3D11_TEXTURE2D_DESC textureUploadDescription;
    textureUploadDescription.MipLevels = 1;
    textureUploadDescription.ArraySize = 1;
	textureUploadDescription.SampleDesc.Count = 1;
	textureUploadDescription.SampleDesc.Quality = 0;
	textureUploadDescription.Usage = D3D11_USAGE_STAGING;
    textureUploadDescription.BindFlags = 0;
	textureUploadDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    textureUploadDescription.MiscFlags = 0;
	textureUploadDescription.Width = TEXTURE_WIDTH;
	textureUploadDescription.Height = TEXTURE_HEIGHT;
	textureUploadDescription.Format = Format;
	return m_Device->CreateTexture2D(&textureUploadDescription,nullptr,texture);
}

HRESULT DX11Player::CreateStagingBuffer(int Width, int Height, DXGI_FORMAT Format, ID3D11Buffer ** buffer){
	D3D11_BUFFER_DESC dynamicBufferDescription;
	dynamicBufferDescription.BindFlags = 0;
	dynamicBufferDescription.ByteWidth = BUFFER_SIZE_BYTES;
	dynamicBufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	dynamicBufferDescription.MiscFlags = 0;
	dynamicBufferDescription.StructureByteStride = 0;
	dynamicBufferDescription.Usage = D3D11_USAGE_STAGING;
	OutputDebugString( L"creating staging buffers" );
	return m_Device->CreateBuffer(&dynamicBufferDescription,nullptr,buffer);
}

HRESULT DX11Player::CreateCopyBufferIn(int Width, int Height, DXGI_FORMAT Format, ID3D11Buffer ** buffer){
	D3D11_BUFFER_DESC dynamicBufferDescription;
	dynamicBufferDescription.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	dynamicBufferDescription.ByteWidth = BUFFER_SIZE_BYTES;
	dynamicBufferDescription.CPUAccessFlags = 0;
	dynamicBufferDescription.MiscFlags = 0;
	dynamicBufferDescription.StructureByteStride = 0;
	dynamicBufferDescription.Usage = D3D11_USAGE_DEFAULT;
	OutputDebugString( L"creating staging buffers" );
	return m_Device->CreateBuffer(&dynamicBufferDescription,nullptr,buffer);
}

HRESULT DX11Player::CreateCopyBufferOut(int Width, int Height, DXGI_FORMAT Format, ID3D11Buffer ** buffer){
	D3D11_BUFFER_DESC dynamicBufferDescription;
	dynamicBufferDescription.BindFlags = 0;
	dynamicBufferDescription.ByteWidth = BUFFER_SIZE_BYTES;
	dynamicBufferDescription.CPUAccessFlags = 0;
	dynamicBufferDescription.MiscFlags = 0;
	dynamicBufferDescription.StructureByteStride = 0;
	dynamicBufferDescription.Usage = D3D11_USAGE_DEFAULT;
	OutputDebugString( L"creating staging buffers" );
	return m_Device->CreateBuffer(&dynamicBufferDescription,nullptr,buffer);
}


void DX11Player::OnRender(){
	
	//TODO: meassure time and consume as many frames as corresponds
	auto now = HighResClock::now();
	auto frameDuration = now - m_LastFrame;
	m_LastFrame = now;
	auto numFrames = std::chrono::duration_cast<std::chrono::microseconds>(frameDuration).count()/1000000.0 * double(FPS) + m_FramesRemainder;
	int framesToconsume = int(numFrames);
	m_FramesRemainder = numFrames - framesToconsume;
	
	
	m_ConsumedBuffers.clear();

	m_NextRenderBuffer = -1;
	while(framesToconsume && m_ReadyToRender.try_recv(m_NextRenderBuffer)){
		framesToconsume -= 1;
		m_ConsumedBuffers.push_back(m_NextRenderBuffer);
	}
	//m_FramesRemainder += framesToconsume;
	if(framesToconsume) m_FramesRemainder = 0;

	/*std::vector<int> consumed_buffers;
	m_NextRenderBuffer = -1;
	m_ReadyToRender.try_recv(m_NextRenderBuffer);*/
	if(m_NextRenderBuffer!=-1){

		//consumed_buffers.push_back(m_NextRenderBuffer);
		m_Context->Unmap(m_UploadBuffers[m_NextRenderBuffer], 0);

		m_Context->CopyResource(m_CopyTextureIn[0],m_UploadBuffers[m_NextRenderBuffer]);
		m_Context->Dispatch(TEXTURE_WIDTH/32,TEXTURE_HEIGHT/30,1);
		m_Context->CopyResource(m_OutputTextures[m_CurrentOutFront], m_CopyTextureOut[0]);
		
		m_Context->Map(m_UploadBuffers[m_NextRenderBuffer],0,D3D11_MAP_WRITE,0,&m_MappedBuffers[m_NextRenderBuffer]);

		/*m_CurrentOutBack+=1;
		m_CurrentOutBack%=m_OutputTextures.size();
		m_CurrentOutFront+=1;
		m_CurrentOutFront%=m_OutputTextures.size();*/
	}

	for(auto buffer: m_ConsumedBuffers){
		m_ReadyToUpload.send(buffer);
	}
}


HANDLE DX11Player::GetSharedHandle(){
	return m_SharedHandles[m_OutputTextures[m_CurrentOutFront]];
}
	
ID3D11Texture2D * DX11Player::GetTexturePointer(){
	return m_OutputTextures[m_CurrentOutFront];
}
	
ID3D11Texture2D * DX11Player::GetRenderTexturePointer(){
	/*if(m_NextRenderBuffer!=-1){
		return m_UploadBuffers[m_NextRenderBuffer];
	}else{*/
		return nullptr;
	//}
}

void DX11Player::DoneRender(){
	if(m_NextRenderBuffer!=-1){
		m_Context->Map(m_UploadBuffers[m_NextRenderBuffer],0,D3D11_MAP_WRITE,0,&m_MappedBuffers[m_NextRenderBuffer]);
		m_ReadyToUpload.send(m_NextRenderBuffer);
	}
}


int DX11Player::GetUploadBufferSize()
{
	return m_ReadyToUpload.size();
}

int DX11Player::GetWaitBufferSize()
{
	return m_ReadyToWait.size();
}

int DX11Player::GetRenderBufferSize()
{
	return m_ReadyToRender.size();
}

extern "C"{
	NATIVE_API DX11HANDLE DX11Player_Create(DX11HANDLE device, const char * directory)
	{
		try{
			return new DX11Player(static_cast<ID3D11Device*>(device),directory);
		}catch(std::exception & e){
			OutputDebugStringA( e.what() );
			return nullptr;
		}
	}

	NATIVE_API void DX11Player_Destroy(DX11HANDLE player)
	{
		delete static_cast<DX11Player*>(player);
	}

	NATIVE_API void DX11Player_OnRender(DX11HANDLE player)
	{
		static_cast<DX11Player*>(player)->OnRender();
	}

	NATIVE_API HANDLE DX11Player_GetSharedHandle(DX11HANDLE player)
	{
		return static_cast<DX11Player*>(player)->GetSharedHandle();
	}

	NATIVE_API DX11HANDLE DX11Player_GetTexturePointer(DX11HANDLE player)
	{
		return static_cast<DX11Player*>(player)->GetTexturePointer();
	}
	
	NATIVE_API DX11HANDLE DX11Player_GetRenderTexturePointer(DX11HANDLE player)
	{
		return static_cast<DX11Player*>(player)->GetRenderTexturePointer();
	}

	NATIVE_API void DX11Player_DoneRender(DX11HANDLE player)
	{
		static_cast<DX11Player*>(player)->DoneRender();
	}

	NATIVE_API int DX11Player_GetUploadBufferSize(DX11HANDLE player)
	{
		return static_cast<DX11Player*>(player)->GetUploadBufferSize();
	}

	NATIVE_API int DX11Player_GetWaitBufferSize(DX11HANDLE player)
	{
		return static_cast<DX11Player*>(player)->GetWaitBufferSize();
	}

	NATIVE_API int DX11Player_GetRenderBufferSize(DX11HANDLE player)
	{
		return static_cast<DX11Player*>(player)->GetRenderBufferSize();
	}

}
