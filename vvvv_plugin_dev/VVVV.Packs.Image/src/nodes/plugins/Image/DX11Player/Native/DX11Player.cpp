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
#include "Timer.h"
#include <memory>
#include "DDS.h"
#include <assert.h>

using namespace DirectX;

static const int HEADER_SIZE_BYTES = 128;
//static const int TEXTURE_WIDTH = 3840;
//static const int TEXTURE_HEIGHT = 2160;
//static const int TEXTURE_SIZE_BYTES = TEXTURE_WIDTH * TEXTURE_HEIGHT * 4;
//static const int BUFFER_SIZE_BYTES = TEXTURE_SIZE_BYTES + HEADER_SIZE_BYTES;
static const int RING_BUFFER_SIZE = 8;
static const int OUTPUT_BUFFER_SIZE = 4;

static bool IsFrameLate(const HighResClock::time_point & presentationTime, int fps_video, HighResClock::time_point now, HighResClock::duration max_lateness){
	auto duration = std::chrono::nanoseconds(1000000000/fps_video);
	return presentationTime+duration+max_lateness<now;
}

static bool IsFrameReady(const HighResClock::time_point & presentationTime, int fps_video, int fps_thread, HighResClock::time_point now, HighResClock::duration max_lateness){
	auto duration_video = std::chrono::nanoseconds(1000000000/fps_video);
	auto duration_thread = std::chrono::nanoseconds(1000000000/fps_thread);
	return presentationTime<=now+duration_thread && now<=presentationTime+duration_video+max_lateness;
}

DX11Player::DX11Player(ID3D11Device * device, const std::string & directory)
	:m_Directory(directory)
	,m_Device(device)
	,m_Context(nullptr)
	,m_CopyTextureIn(RING_BUFFER_SIZE)
	,m_UploadBuffers(RING_BUFFER_SIZE)
	,m_Overlaps(RING_BUFFER_SIZE)
	,m_WaitEvents(RING_BUFFER_SIZE)
	,m_FileHandles(RING_BUFFER_SIZE)
	,m_MappedBuffers(RING_BUFFER_SIZE)
	,m_UploaderThreadRunning(false)
	,m_WaiterThreadRunning(false)
	,m_RateThreadRunning(false)
	,m_CurrentFrame(0)
	,m_CurrentOutFront(RING_BUFFER_SIZE/2)
	,m_CurrentOutBack(0)
	,m_DroppedFrames(0)
	,m_Fps(60)
{
	HRESULT hr;

	// list all files in folder
	// TODO: filter by extension
	tinydir_dir dir;
	tinydir_open_sorted(&dir, directory.c_str());
	std::stringstream str;
	str << "reading folder " <<  directory <<  " with " << dir.n_files << " files" << std::endl;
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

	// if no files finish
	if(m_ImageFiles.empty()){
		throw std::exception(("no images found in " + directory).c_str());
	}

	// parse first image header and assume all have the same format
	auto data = std::unique_ptr<uint8_t[]>();
	DDS_HEADER * header;
	uint8_t * dataptr;
	size_t size;
    TexMetadata mdata;
	GetMetadataFromDDSFile(m_ImageFiles[0].c_str(),DDS_FLAGS_NONE, mdata);
	m_Width = mdata.width;
	m_Height = mdata.height;
	auto format = mdata.format;
	auto offset = mdata.bytesHeader;
	size_t numbytesdata = mdata.bytesData;
	size_t rowpitch = mdata.rowPitch;

	m_CopyBox.back = 1;
	m_CopyBox.bottom = m_Height + 4;
	m_CopyBox.front = 0;
	m_CopyBox.left = 0;
	m_CopyBox.right = m_Width;
	m_CopyBox.top = 4;
		
	//size_t numbytesfile = numbytesdata + offset;
	std::stringstream ss;
	ss << "loading " << m_ImageFiles.size() << " images from " << directory << " " << m_Width << "x" << m_Height << " " << format << " with " << numbytesdata << " bytes per file and " << offset << " offset, rowpitch: " << rowpitch << std::endl;
	OutputDebugStringA( ss.str().c_str() ); 


	// create dx device in non passed or get the 
	// immediate context of the one passed by parameter
	if(device==nullptr){
		OutputDebugString( L"creating device\n" );
		D3D_FEATURE_LEVEL level;
		hr = D3D11CreateDevice(nullptr,
			D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			D3D11_CREATE_DEVICE_FLAG::D3D11_CREATE_DEVICE_SINGLETHREADED,
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

	// init mapped buffers to 0
	OutputDebugString( L"zeroing read back data\n" );
	for(auto & map: m_MappedBuffers){
		ZeroMemory(&map,sizeof(D3D11_MAPPED_SUBRESOURCE));
	}
	

	// create the copy textures, used to copy from the upload
	// buffers and shared with the application or plugin
	OutputDebugString( L"creating staging textures\n" );
	D3D11_TEXTURE2D_DESC textureDescriptionCopy;
	textureDescriptionCopy.MipLevels = 1;
	textureDescriptionCopy.ArraySize = 1;
	textureDescriptionCopy.SampleDesc.Count = 1;
	textureDescriptionCopy.SampleDesc.Quality = 0;
	textureDescriptionCopy.Usage = D3D11_USAGE_DEFAULT;
	textureDescriptionCopy.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	textureDescriptionCopy.CPUAccessFlags = 0;
	textureDescriptionCopy.Width = m_Width;
	textureDescriptionCopy.Height = m_Height;
	textureDescriptionCopy.Format = format;
	if(device==nullptr){
		textureDescriptionCopy.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
	}else{
		textureDescriptionCopy.MiscFlags = 0;
	}
	for(size_t i=0;i<m_CopyTextureIn.size();i++){
		hr = m_Device->CreateTexture2D(&textureDescriptionCopy,nullptr,&m_CopyTextureIn[i]);

		if(!device){
			// if using our own device, create a shared handle for each texture
			OutputDebugString( L"getting shared texture handle\n" );
			IDXGIResource* pTempResource(NULL);
			HANDLE sharedHandle;
			hr = m_CopyTextureIn[i]->QueryInterface( __uuidof(IDXGIResource), (void**)&pTempResource );
			if(FAILED(hr)){
				throw std::exception("Coudln't query interface");
			}
			hr = pTempResource->GetSharedHandle(&sharedHandle);
			if(FAILED(hr)){
				throw std::exception("Coudln't get shared handle");
			}
			m_SharedHandles[m_CopyTextureIn[i]] = sharedHandle;
			pTempResource->Release();
		}
	}
	

	// create the upload buffers, we upload directly to these from the disk
	// using 4 more rows than the original image to avoid the header. BC formats
	// use a 4x4 tile so we write the header at the end of the first 4 rows and 
	// then skip it when copying to the copy textures
	for(size_t i = 0; i < m_UploadBuffers.size(); i++){
		hr = CreateStagingTexture(m_Width,m_Height+4,format,&m_UploadBuffers[i]);
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

	
	// map all the upload buffers and send them to the uploader thread
	for(int i=0;i<RING_BUFFER_SIZE;i++){
		Frame nextFrame;
		nextFrame.idx = i;
		m_Context->Map(m_UploadBuffers[i],0,D3D11_MAP_WRITE,0,&m_MappedBuffers[i]);
		m_ReadyToUpload.send(nextFrame);
		m_WaitEvents[i] = CreateEventW(0,false,false,0);
		ZeroMemory(&m_Overlaps[i],sizeof(OVERLAPPED));
	}

	// uploader thread: reads async from disk to mapped GPU memory
	// and sends an event to wait on to the waiter thread
	OutputDebugString( L"creating upload thread\n" );
	m_UploaderThreadRunning = true;
	auto running = &m_UploaderThreadRunning;
	auto currentFrame = &m_CurrentFrame;
	auto imageFiles = &m_ImageFiles;
	auto readyToUpload = &m_ReadyToUpload;
	auto readyToWait = &m_ReadyToWait;
	auto readyToRender = &m_ReadyToRender;
	auto readyToRate = &m_ReadyToRate;
	auto mappedBuffers = &m_MappedBuffers;
	auto overlaps = &m_Overlaps;
	auto waitEvents = &m_WaitEvents;
	auto fileHandlers = &m_FileHandles;
	auto fps = &m_Fps;
	m_UploaderThread = std::thread([numbytesdata,rowpitch,offset,running,currentFrame,imageFiles,readyToUpload,readyToWait,readyToRender,mappedBuffers,overlaps,waitEvents,fileHandlers,fps]{
		auto nextFrameTime = HighResClock::now();
		while(*running){
			Frame nextFrame;
			nextFrame.nextToLoad = -1;
			readyToUpload->recv(nextFrame);
			if(nextFrame.nextToLoad==-1){
				nextFrame.nextToLoad = *currentFrame;
			}else{
				*currentFrame = nextFrame.nextToLoad;
			}
			auto ptr = (char*)mappedBuffers->at(nextFrame.idx).pData;
			//FIXME!!!
			ptr += rowpitch*4-offset;
			if(ptr){
				auto file = CreateFileA(imageFiles->at(nextFrame.nextToLoad).c_str(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED,NULL);
				if (file != INVALID_HANDLE_VALUE) {
					ZeroMemory(&overlaps->at(nextFrame.idx),sizeof(OVERLAPPED));
					overlaps->at(nextFrame.idx).hEvent = waitEvents->at(nextFrame.idx);
					fileHandlers->at(nextFrame.idx) = file;
					DWORD bytesRead=0;
					DWORD totalBytesRead=0;
					DWORD totalToRead = numbytesdata;
					//SetFilePointer(file,HEADER_SIZE_BYTES,nullptr,FILE_BEGIN);
					//while(totalBytesRead<totalToRead){
						ReadFile(file,ptr+totalBytesRead,totalToRead - totalBytesRead,&bytesRead,&overlaps->at(nextFrame.idx));
						//totalBytesRead += bytesRead;
					//}
					nextFrame.presentationTime = nextFrameTime;
					readyToWait->send(nextFrame);
				}
				
				auto now = HighResClock::now();
				do{
					nextFrameTime += std::chrono::nanoseconds((uint64_t)floor(1000000000/double(*fps)+0.5));
					(*currentFrame)+=1;
					(*currentFrame)%=imageFiles->size();
				}while(nextFrameTime<now);
			}
		} 
	});
	
	// waiter thread: waits for the transfer from disk to GPU mem to finish
	// and sends the frame num to the rate thread
	OutputDebugString( L"creating waiter thread\n" );
	m_WaiterThreadRunning = true;
	running = &m_WaiterThreadRunning;
	auto context = m_Context;
	auto copyTextureIn = &m_CopyTextureIn;
	auto uploadBuffers = &m_UploadBuffers;
	auto currentBack = &m_CurrentOutBack;
	m_WaiterThread = std::thread([running,readyToWait,readyToRate,readyToRender,waitEvents,fileHandlers,context,copyTextureIn,uploadBuffers,mappedBuffers,currentBack,fps]{
		auto start = HighResClock::now();
		while(*running){
			Frame nextFrame;
			readyToWait->recv(nextFrame);
			auto waitEvent = waitEvents->at(nextFrame.idx);
			WaitForSingleObject(waitEvent,INFINITE);
			readyToRate->send(nextFrame);
			CloseHandle(fileHandlers->at(nextFrame.idx));
		}
	});


	// rate thread: receives frames already in the GPU and controls
	// the playback rate, once a frame is on time to be shown it sends it
	// to the main thread
	m_RateThreadRunning = true;
	running = &m_RateThreadRunning;
	auto droppedFrames = &m_DroppedFrames;
	m_RateThread = std::thread([running,readyToRate,readyToUpload,readyToRender,fps,droppedFrames] {
		const auto max_lateness = std::chrono::milliseconds(2);
		Frame nextFrame;
		nextFrame.idx = -1;
		Timer timer(std::chrono::nanoseconds(1000000000 / *fps));
		auto prevFps = *fps;
		while(*running){
			auto now = HighResClock::now();
			std::vector<Frame> consumedBuffers;
			bool ready = false;
			bool late = true;
			if(nextFrame.idx != -1){
				ready = IsFrameReady(nextFrame.presentationTime,*fps,*fps,now,max_lateness);
				late = IsFrameLate(nextFrame.presentationTime,*fps,now,max_lateness);
				if(!ready && !late){
					continue;
				}else{
					consumedBuffers.push_back(nextFrame);
					nextFrame.idx = -1;
				}
			}

			while (late && readyToRate->try_recv(nextFrame)){
				ready = IsFrameReady(nextFrame.presentationTime,*fps,60,now,max_lateness);
				if(ready){
					consumedBuffers.push_back(nextFrame);
					nextFrame.idx = -1;
					break;
				}else{
					late = IsFrameLate(nextFrame.presentationTime,*fps,now,max_lateness);
					if(late){
						consumedBuffers.push_back(nextFrame);
						nextFrame.idx = -1;
					}
				}
			}

			if(!consumedBuffers.empty() && ready){
				readyToRender->send(consumedBuffers.back());
				*droppedFrames+=int(consumedBuffers.size())-1;
				consumedBuffers.pop_back();
			}

			for(auto buffer: consumedBuffers){
				readyToUpload->send(buffer);
			}

			if(prevFps!=*fps){
				timer.set_period(std::chrono::nanoseconds(1000000000 / *fps));
				prevFps = *fps;
			}
			timer.wait_next();
		}
	});

	m_NextRenderFrame.idx = -1;
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
	textureUploadDescription.Width = Width;
	textureUploadDescription.Height = Height;
	textureUploadDescription.Format = Format;
	return m_Device->CreateTexture2D(&textureUploadDescription,nullptr,texture);
}


void DX11Player::OnRender(){
	Frame frame;
	auto now = HighResClock::now();
	auto max_lateness = std::chrono::milliseconds(2);
	bool late = true;
	std::vector<Frame> consumedBuffers;
	while(late && m_ReadyToRender.try_recv(frame)){
		consumedBuffers.push_back(frame);
		late = IsFrameLate(frame.presentationTime,m_Fps,now,max_lateness);
	}

	if(!consumedBuffers.empty()){
		m_NextRenderFrame = consumedBuffers.back();
		m_Context->Unmap(m_UploadBuffers[m_NextRenderFrame.idx], 0);

		m_Context->CopySubresourceRegion(m_CopyTextureIn[m_CurrentOutFront],0,0,0,0,m_UploadBuffers[m_NextRenderFrame.idx],0,&m_CopyBox);
		
		m_Context->Map(m_UploadBuffers[m_NextRenderFrame.idx],0,D3D11_MAP_WRITE,0,&m_MappedBuffers[m_NextRenderFrame.idx]);
		m_DroppedFrames+=int(consumedBuffers.size())-1;
	}

	for(auto buffer: consumedBuffers){
		buffer.nextToLoad = -1;
		m_ReadyToUpload.send(buffer);
	}
}


HANDLE DX11Player::GetSharedHandle(){
	return m_SharedHandles[m_CopyTextureIn[m_CurrentOutFront]];
}
	
ID3D11Texture2D * DX11Player::GetTexturePointer(){
	return m_CopyTextureIn[m_CurrentOutFront];
}
	
ID3D11Texture2D * DX11Player::GetRenderTexturePointer(){
	/*if(m_NextRenderBuffer!=-1){
		return m_UploadBuffers[m_NextRenderBuffer];
	}else{*/
		return nullptr;
	//}
}

std::string DX11Player::GetDirectory() const
{
	return m_Directory;
}

int DX11Player::GetUploadBufferSize() const
{
	return m_ReadyToUpload.size();
}

int DX11Player::GetWaitBufferSize() const
{
	return m_ReadyToWait.size();
}

int DX11Player::GetRenderBufferSize() const
{
	return m_ReadyToRender.size();
}

int DX11Player::GetDroppedFrames() const
{
	return m_DroppedFrames;
}


size_t DX11Player::GetCurrentLoadFrame() const{
	return m_CurrentFrame;
}


size_t DX11Player::GetCurrentRenderFrame() const{
	return m_NextRenderFrame.nextToLoad;
}


void DX11Player::SetFPS(int fps){
	m_Fps = fps;
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

	NATIVE_API const char * DX11Player_GetDirectory(DX11HANDLE player)
	{
		return static_cast<DX11Player*>(player)->GetDirectory().c_str();
	}

	NATIVE_API int DX11Player_DirectoryHasChanged(DX11HANDLE player, const char * dir)
	{
		return static_cast<DX11Player*>(player)->GetDirectory() != std::string(dir);
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

	NATIVE_API int DX11Player_GetDroppedFrames(DX11HANDLE player)
	{
		return static_cast<DX11Player*>(player)->GetDroppedFrames();
	}

	NATIVE_API int DX11Player_GetCurrentLoadFrame(DX11HANDLE player)
	{
		return static_cast<DX11Player*>(player)->GetCurrentLoadFrame();
	}

	NATIVE_API int DX11Player_GetCurrentRenderFrame(DX11HANDLE player)
	{
		return static_cast<DX11Player*>(player)->GetCurrentRenderFrame();
	}
	
	NATIVE_API void DX11Player_SetFPS(DX11HANDLE player, int fps)
	{
		static_cast<DX11Player*>(player)->SetFPS(fps);
	}

}
