// Native.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "DX11Player.h"
#include <comdef.h>
#include <sstream>
#include "Timer.h"
#include <DirectXMath.h>
#include "VShader.h"
#include "RGB_to_RGBA.h";
#include "BGR_to_RGBA.h";
#include "A2R10G10B10_to_R10G10B10A2.h"
#include "PSCbYCr888_to_RGBA8888.h"
#include "PSCbYCr101010_to_R10G10B10A2.h"
#include "PSCbYCr161616_to_RGBA16161616.h"
#include <WinIoCtl.h>

#pragma comment(lib, "Kernel32.lib")


static const int RING_BUFFER_SIZE = 8;
static const int OUTPUT_BUFFER_SIZE = 4;


static bool IsFrameLate(const HighResClock::time_point & presentationTime, int fps_video, HighResClock::time_point now, HighResClock::duration max_lateness){
	if(fps_video==0) return false;
	auto duration = std::chrono::nanoseconds(1000000000/fps_video);
	return presentationTime+duration+max_lateness<now;
}

static bool IsFrameReady(const HighResClock::time_point & presentationTime, int fps_video, int fps_thread, HighResClock::time_point now, HighResClock::duration max_lateness){
	if(fps_video==0 || fps_thread==0) return false;
	auto duration_video = std::chrono::nanoseconds(1000000000/fps_video);
	auto duration_thread = std::chrono::nanoseconds(1000000000/fps_thread);
	return presentationTime<=now+duration_thread && now<=presentationTime+duration_video+max_lateness;
}

static uint32_t SectorSize(char cDisk)
{
	HANDLE hDevice;

    // Build the logical drive path and get the drive device handle
    std::wstring logicalDrive = L"\\\\.\\";
    wchar_t drive[3];
    drive[0] = cDisk;
    drive[1] = L':';
    drive[2] = L'\0';
    logicalDrive.append(drive);

    hDevice = CreateFile(
        logicalDrive.c_str(),
        0, 
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if (hDevice == INVALID_HANDLE_VALUE)
    {
		throw std::exception("Error finding out disk sector size");
        return -1;
    }   

    // Now that we have the device handle for the disk, let us get disk's metadata
    DWORD outsize;
    STORAGE_PROPERTY_QUERY storageQuery;
    memset(&storageQuery, 0, sizeof(STORAGE_PROPERTY_QUERY));
    storageQuery.PropertyId = StorageAccessAlignmentProperty;
    storageQuery.QueryType  = PropertyStandardQuery;

    STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR diskAlignment = {0};
    memset(&diskAlignment, 0, sizeof(STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR));

    if (!DeviceIoControl(hDevice, 
        IOCTL_STORAGE_QUERY_PROPERTY, 
        &storageQuery, 
        sizeof(STORAGE_PROPERTY_QUERY), 
        &diskAlignment,
        sizeof(STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR), 
        &outsize,
        NULL)
        )
    {
		//throw std::exception("Error finding out disk sector size");
        //return -1;
		return 512;
    }

	return diskAlignment.BytesPerLogicalSector;
}

size_t NextMultiple(size_t in, size_t multiple){
	return in + (multiple - in % multiple);
}

DX11Player::DX11Player(const std::string & directory)
	:m_Device(nullptr)
	,m_Context(nullptr)
	,m_CopyTextureIn(OUTPUT_BUFFER_SIZE,nullptr)
	,m_ShaderResourceViews(OUTPUT_BUFFER_SIZE,nullptr)
	,m_BackBuffer(nullptr)
	,m_RenderTargetView(nullptr)
	,m_TextureBack(nullptr)
	,m_UploaderThreadRunning(false)
	,m_WaiterThreadRunning(false)
	,m_RateThreadRunning(false)
	,m_FramePool(RING_BUFFER_SIZE)
	,m_ExternalRate(false)
	,m_InternalRate(true)
	,m_CurrentOutFront(OUTPUT_BUFFER_SIZE/2)
	,m_DroppedFrames(0)
	,m_Fps(60)
	,m_CurrentFrame(0)
	,m_AvgDecodeDuration(0)
	,m_AvgPipelineLatency(std::chrono::milliseconds(1000/m_Fps * 2))
{
	HRESULT hr;

	// create a dx11 device
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
	
	

	// uploader thread: reads async from disk to mapped GPU memory
	// and sends an event to wait on to the waiter thread
	OutputDebugString( L"creating upload thread\n" );
	auto running = &m_UploaderThreadRunning;
	auto currentFrame = &m_CurrentFrame;
	auto sequence_ptr = &m_Sequence;
	auto readyToUpload = &m_ReadyToUpload;
	auto readyToWait = &m_ReadyToWait;
	auto readyToRender = &m_ReadyToRender;
	auto readyToRate = &m_ReadyToRate;
	auto fps = &m_Fps;
	auto externalRate = &m_ExternalRate;
	auto nextFrameChannel = &m_NextFrameChannel;
	auto internalRate = &m_InternalRate;
	auto copyBox = &m_CopyBox;
	auto self = this;
	m_UploaderThread = std::thread([running,copyBox,directory,self,currentFrame,sequence_ptr,readyToUpload,readyToWait,fps,externalRate,nextFrameChannel,internalRate]{
		*sequence_ptr = std::make_shared<ImageSequence>(directory);
		auto sequence = *sequence_ptr;

		// Add enough bytes to read the header + data but we need to read
		// a multiple of the physical sector size since we are reading 
		// directly from disk with no buffering
		auto sectorSize = SectorSize('d');
		auto numbytesdata = NextMultiple(sequence->BytesData()+sequence->DataOffset(),sectorSize);

		// box to copy upload buffer to texture skipping 4 first rows
		// to skip header
		copyBox->back = 1;
		copyBox->bottom = sequence->Height() + 4;
		copyBox->front = 0;
		copyBox->left = 0;
		copyBox->right = sequence->InputWidth();
		copyBox->top = 4;
	
		HRESULT hr;
		// init the ring buffer:
		// - map all the upload buffers and send them to the uploader thread.
		// - create the upload buffers, we upload directly to these from the disk
		// using 4 more rows than the original image to avoid the header. BC formats
		// use a 4x4 tile so we write the header at the end of the first 4 rows and 
		// then skip it when copying to the copy textures
		for(int i=0;i<RING_BUFFER_SIZE;i++){
			Frame & nextFrame = self->m_FramePool[i];
			nextFrame.idx = i;
			nextFrame.waitEvent = CreateEventW(0,false,false,0);
			hr = self->CreateStagingTexture(sequence->InputWidth(),sequence->Height()+4,sequence->TextureFormat(),&nextFrame.uploadBuffer);
			if(FAILED(hr)){
				_com_error error(hr);
				auto msg = error.ErrorMessage();

				auto lpBuf = new char[1024];
				ZeroMemory(lpBuf,sizeof(lpBuf));
				FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), lpBuf, 0, NULL);
				throw std::exception((std::string("Coudln't create staging texture") + lpBuf).c_str());
			}
			self->m_Context->Map(nextFrame.uploadBuffer,0,D3D11_MAP_WRITE,0,&nextFrame.mappedBuffer);
			self->m_ReadyToUpload.send(&nextFrame);
		}

		// create the copy textures, used to copy from the upload
		// buffers and shared with the application or plugin
		// if the input format is directly supported or
		// to pass to the colorspace conversion shader
		OutputDebugString( L"creating output textures\n" );
		D3D11_TEXTURE2D_DESC textureDescriptionCopy;
		textureDescriptionCopy.MipLevels = 1;
		textureDescriptionCopy.ArraySize = 1;
		textureDescriptionCopy.SampleDesc.Count = 1;
		textureDescriptionCopy.SampleDesc.Quality = 0;
		textureDescriptionCopy.Usage = D3D11_USAGE_DEFAULT;
		textureDescriptionCopy.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		textureDescriptionCopy.CPUAccessFlags = 0;
		textureDescriptionCopy.Width = sequence->InputWidth();
		textureDescriptionCopy.Height = sequence->Height();
		textureDescriptionCopy.Format = sequence->TextureFormat();
		if(sequence->InputFormat() == ImageSequence::DX11_NATIVE){
			textureDescriptionCopy.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
		}else{
			textureDescriptionCopy.MiscFlags = 0;
		}
		for(size_t i=0;i<self->m_CopyTextureIn.size();i++){
			hr = self->m_Device->CreateTexture2D(&textureDescriptionCopy,nullptr,&self->m_CopyTextureIn[i]);
			// create a shared handle for each texture
			if(sequence->InputFormat() == ImageSequence::DX11_NATIVE){
				OutputDebugString( L"getting shared texture handle\n" );
				IDXGIResource* pTempResource(NULL);
				HANDLE sharedHandle;
				hr = self->m_CopyTextureIn[i]->QueryInterface( __uuidof(IDXGIResource), (void**)&pTempResource );
				if(FAILED(hr)){
					throw std::exception("Coudln't query interface\n");
				}
				hr = pTempResource->GetSharedHandle(&sharedHandle);
				if(FAILED(hr)){
					throw std::exception("Coudln't get shared handle\n");
				}
				self->m_SharedHandles[self->m_CopyTextureIn[i]] = sharedHandle;
				pTempResource->Release();
			}
		}


		// If the input format is not directly supported by DX11
		// create a shader to process them and output the data
		// as RGBA
		if (sequence->InputFormat() != ImageSequence::DX11_NATIVE)
		{
			OutputDebugStringA("Format not directly supported setting up colorspace conversion shader");

			// figure out the conversion shaders from the 
			// image sequence input format and depth
			const BYTE * pixelShaderSrc;
			size_t sizePixelShaderSrc;
			bool useSampler = true;

			switch(sequence->InputFormat()){
			case ImageSequence::ARGB:
				if(sequence->InputDepth() == 10){
					pixelShaderSrc = A2R10G10B10_to_R10G10B10A2;
					sizePixelShaderSrc = sizeof(A2R10G10B10_to_R10G10B10A2);
					useSampler = false;
				}else{
					throw std::exception("ARGB conversion only supported for 10bits");
				}
				break;
			
			case ImageSequence::BGR:
				pixelShaderSrc = BGR_to_RGBA;
				sizePixelShaderSrc = sizeof(BGR_to_RGBA);
				break;

			case ImageSequence::CbYCr:
				switch(sequence->InputDepth()){
				case 8:
					pixelShaderSrc = PSCbYCr888_to_RGBA8888;
					sizePixelShaderSrc = sizeof(PSCbYCr888_to_RGBA8888);
					break;
				case 10:
					pixelShaderSrc = PSCbYCr101010_to_R10G10B10A2;
					sizePixelShaderSrc = sizeof(PSCbYCr101010_to_R10G10B10A2);
					useSampler = false;
					break;
				case 16:
					pixelShaderSrc = PSCbYCr161616_to_RGBA16161616;
					sizePixelShaderSrc = sizeof(PSCbYCr161616_to_RGBA16161616);
					break;
				}
				break;
			

			case ImageSequence::RGB:
				pixelShaderSrc = RGB_to_RGBA;
				sizePixelShaderSrc = sizeof(RGB_to_RGBA);
				break;
			}

			// Set the viewport to the size of the image
			D3D11_VIEWPORT viewport = {0.0f, 0.0f, float(sequence->Width()), float(sequence->Height()), 0.0f, 1.0f};
			self->m_Context->RSSetViewports(1,&viewport); 
			OutputDebugString( L"Set viewport" );

			// Create Pixel and Vertex shaders
			ID3D11VertexShader * vertexShader;
			ID3D11PixelShader * pixelShader;
			hr = self->m_Device->CreateVertexShader(VShader,sizeof(VShader),nullptr,&vertexShader);
			if(FAILED(hr)){
				throw std::exception("Coudln't create vertex shader\n");
			}
			OutputDebugString( L"Created vertex shader\n" );

			hr = self->m_Device->CreatePixelShader(pixelShaderSrc,sizePixelShaderSrc,nullptr,&pixelShader);
			if(FAILED(hr)){
				throw std::exception("Coudln't create pixel shader\n");
			}
			OutputDebugString( L"Created pixel shader\n" );

			self->m_Context->VSSetShader(vertexShader,nullptr,0);
			self->m_Context->PSSetShader(pixelShader,nullptr,0);
			OutputDebugString( L"Created vertex and pixel shaders\n" );

			vertexShader->Release();
			pixelShader->Release();

			// Create a quad with the size of the image to
			// render the color converted texture
			struct Vertex
			{
				DirectX::XMFLOAT3 position;
				DirectX::XMFLOAT2 texcoord;
				Vertex(const DirectX::XMFLOAT3 &position, const DirectX::XMFLOAT2 &texcoord)
					:position(position)
					,texcoord(texcoord){}
			};

			D3D11_BUFFER_DESC vertexBufferDescription;
			vertexBufferDescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			vertexBufferDescription.ByteWidth = 6 * sizeof(Vertex);
			vertexBufferDescription.CPUAccessFlags = 0;
			vertexBufferDescription.MiscFlags = 0;
			vertexBufferDescription.StructureByteStride = 0;
			vertexBufferDescription.Usage = D3D11_USAGE_DEFAULT;
		
			Vertex quad[] = {
				Vertex(DirectX::XMFLOAT3(-1.0f, 1.0f, 0.5f),DirectX::XMFLOAT2(0.0f, 0.0f)),
				Vertex(DirectX::XMFLOAT3(1.0f, 1.0f, 0.5f),DirectX::XMFLOAT2(1.0f, 0.0f)),
				Vertex(DirectX::XMFLOAT3(-1.0f, -1.0f, 0.5f),DirectX::XMFLOAT2(0.0f, 1.0f)),

				Vertex(DirectX::XMFLOAT3(-1.0f, -1.0f, 0.5f),DirectX::XMFLOAT2(0.0f, 1.0f)),
				Vertex(DirectX::XMFLOAT3(1.0f, 1.0f, 0.5f),DirectX::XMFLOAT2(1.0f, 0.0f)),
				Vertex(DirectX::XMFLOAT3(1.0f, -1.0f, 0.5f),DirectX::XMFLOAT2(1.0f, 1.0f)),
			};

			D3D11_SUBRESOURCE_DATA bufferData;
			bufferData.pSysMem = quad;
			bufferData.SysMemPitch = 0;
			bufferData.SysMemSlicePitch = 0;
		
			ID3D11Buffer * vertexBuffer;
			hr = self->m_Device->CreateBuffer(&vertexBufferDescription,&bufferData,&vertexBuffer);
			if(FAILED(hr)){
				throw std::exception("Coudln't create vertex buffer\n");
			}
			OutputDebugString( L"Created vertex buffer\n" );

			D3D11_INPUT_ELEMENT_DESC layout[] = {
				{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
				{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(DirectX::XMFLOAT3), D3D11_INPUT_PER_VERTEX_DATA, 0},
			};

			ID3D11InputLayout * vertexLayout;
			hr = self->m_Device->CreateInputLayout(layout, 2, VShader, sizeof(VShader), &vertexLayout);
			if(FAILED(hr)){
				throw std::exception("Coudln't create input layout\n");
			}
			OutputDebugString( L"Created vertex layout\n" );

			self->m_Context->IASetInputLayout(vertexLayout);
			OutputDebugString( L"Set vertex layout\n" );
			vertexLayout->Release();

			UINT stride = sizeof(Vertex);
			UINT offset = 0;
			self->m_Context->IASetVertexBuffers(0,1,&vertexBuffer,&stride,&offset);
			self->m_Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			OutputDebugString( L"Created vertex buffer and topology\n" );
			vertexBuffer->Release();


			// if required create a sampler to access the texture
			if(useSampler){
				D3D11_SAMPLER_DESC samplerDesc;
				ZeroMemory(&samplerDesc,sizeof(D3D11_SAMPLER_DESC));
				samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
				samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
				samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
				samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
				samplerDesc.MaxLOD = 0;//FLT_MAX;
				samplerDesc.MaxAnisotropy = 0;//16;
				samplerDesc.ComparisonFunc = (D3D11_COMPARISON_FUNC)0;//D3D11_COMPARISON_NEVER;
				ID3D11SamplerState * samplerState;
				hr = self->m_Device->CreateSamplerState(&samplerDesc,&samplerState);
				if(FAILED(hr)){
					throw std::exception("Coudln't create sampler\n");
				}
				OutputDebugString( L"Created sampler state\n" );

				self->m_Context->PSSetSamplers(0,1,&samplerState);
				OutputDebugString( L"Set sampler state\n" );
				samplerState->Release();
			}

			// Create shader constants buffer
			_declspec(align(16))
			struct PS_CONSTANT_BUFFER
			{
				float InputWidth;
				float InputHeight;
				float OutputWidth;
				float OutputHeight;
				float OnePixel;
				float YOrigin;
				float YCoordinateSign;
				bool RequiresSwap;
			};

			PS_CONSTANT_BUFFER constantData;
			constantData.InputWidth = (float)sequence->InputWidth();
			constantData.InputHeight = (float)sequence->Height();
			constantData.OutputWidth = (float)sequence->Width();
			constantData.OutputHeight = (float)sequence->Height();
			constantData.OnePixel = 1.0 / (float)sequence->Width();
			constantData.YOrigin = sequence->RequiresVFlip()?1.0:0.0;
			constantData.YCoordinateSign = sequence->RequiresVFlip()?-1.0:1.0;
			constantData.RequiresSwap = sequence->RequiresByteSwap();

			D3D11_BUFFER_DESC shaderVarsDescription;
			shaderVarsDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			shaderVarsDescription.ByteWidth = sizeof(PS_CONSTANT_BUFFER);
			shaderVarsDescription.CPUAccessFlags = 0;
			shaderVarsDescription.MiscFlags = 0;
			shaderVarsDescription.StructureByteStride = 0;
			shaderVarsDescription.Usage = D3D11_USAGE_DEFAULT;
		
			bufferData.pSysMem = &constantData;
			bufferData.SysMemPitch = 0;
			bufferData.SysMemSlicePitch = 0;

			ID3D11Buffer * varsBuffer;
			hr = self->m_Device->CreateBuffer(&shaderVarsDescription,&bufferData,&varsBuffer);
			if(FAILED(hr)){
				throw std::exception("Coudln't create constant buffer\n");
			}
			OutputDebugString( L"Created constants buffer\n" );
        
			self->m_Context->PSSetConstantBuffers(0,1,&varsBuffer);
			OutputDebugString( L"Set constants buffer\n" );
			varsBuffer->Release();

			// Create shader resource views to be able to access the 
			// source textures
			int i=0;
			for(auto & shaderResourceView: self->m_ShaderResourceViews){
				hr = self->m_Device->CreateShaderResourceView(self->m_CopyTextureIn[i],nullptr,&shaderResourceView);
				if(FAILED(hr)){
					throw std::exception("Coudln't create shader resource view\n");
				}
				i++;
			}
			OutputDebugString( L"Created shader resource views\n" );

		
			// Create a backbuffer texture to render to
			auto backBufferTexDescription = textureDescriptionCopy;
			backBufferTexDescription.MipLevels = 1;
			backBufferTexDescription.ArraySize = 1;
			backBufferTexDescription.SampleDesc.Count = 1;
			backBufferTexDescription.SampleDesc.Quality = 0;
			backBufferTexDescription.Usage = D3D11_USAGE_DEFAULT;
			backBufferTexDescription.BindFlags = D3D11_BIND_RENDER_TARGET;
			backBufferTexDescription.CPUAccessFlags = 0;
			backBufferTexDescription.MiscFlags = 0;
			backBufferTexDescription.Width = self->m_Sequence->Width();
			backBufferTexDescription.Height = self->m_Sequence->Height();
			backBufferTexDescription.Format = self->m_Sequence->TextureOutFormat();

			D3D11_RENDER_TARGET_VIEW_DESC viewDesc;
			ZeroMemory(&viewDesc,sizeof(D3D11_RENDER_TARGET_VIEW_DESC));
			viewDesc.Format = backBufferTexDescription.Format;
			viewDesc.Texture2D.MipSlice = 0;
			viewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

			hr = self->m_Device->CreateTexture2D(&backBufferTexDescription,nullptr,&self->m_BackBuffer);
			if(FAILED(hr)){
				throw std::exception("Coudln't create back buffer texture\n");
			}
			hr = self->m_Device->CreateRenderTargetView(self->m_BackBuffer,&viewDesc,&self->m_RenderTargetView);
			if(FAILED(hr)){
				throw std::exception("Coudln't create render target view\n");
			}
			self->m_Context->OMSetRenderTargets(1,&self->m_RenderTargetView,nullptr);


			textureDescriptionCopy.Width = self->m_Sequence->Width();
			textureDescriptionCopy.Format = self->m_Sequence->TextureOutFormat();
			textureDescriptionCopy.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
			hr = self->m_Device->CreateTexture2D(&textureDescriptionCopy,nullptr,&self->m_TextureBack);
			if(FAILED(hr)){
				throw std::exception("Coudln't create backbuffer texture\n");
			}

			// if using our own device, create a shared handle for each texture
			OutputDebugString( L"getting shared texture handle\n" );
			IDXGIResource* pTempResource(NULL);
			HANDLE sharedHandle;
			hr = self->m_TextureBack->QueryInterface( __uuidof(IDXGIResource), (void**)&pTempResource );
			if(FAILED(hr)){
				throw std::exception("Coudln't query interface\n");
			}
			hr = pTempResource->GetSharedHandle(&sharedHandle);
			if(FAILED(hr)){
				throw std::exception("Coudln't get shared handle\n");
			}
			self->m_SharedHandles[self->m_TextureBack] = sharedHandle;
			pTempResource->Release();

		
		}







		*running = true;
		auto nextFrameTime = HighResClock::now();
		bool paused = false;
		int nextFps = *fps;
		Frame * nextFrame = nullptr;
		while(readyToUpload->recv(nextFrame)){
			auto now = HighResClock::now();
			auto absFps = abs(nextFps);
			if(*externalRate){
				if(!nextFrameChannel->recv(*currentFrame)){
					break;
				}
				now = HighResClock::now();
				nextFrameTime = now;
			}else{
				if(absFps!=0){
					do{
						if(*internalRate){
							nextFrameTime += std::chrono::nanoseconds((uint64_t)floor(1000000000/double(absFps)+0.5));
						}else{
							nextFrameTime = now;
						}
						if(nextFps>0){
							(*currentFrame)+=1;
							(*currentFrame)%=sequence->NumImages();
						}else if(nextFps<0){
							(*currentFrame)-=1;
							(*currentFrame)%=sequence->NumImages();
						}
					}while(nextFrameTime<now);
				}else{
					paused = true;
				}
			}
			*currentFrame%=sequence->NumImages();
			nextFps = *fps;
			if(nextFrame->nextToLoad==-1){
				nextFrame->nextToLoad = *currentFrame;
			}else{
				nextFrame->nextToLoad%=sequence->NumImages();
				*currentFrame = nextFrame->nextToLoad;
			}
			if(paused && nextFps!=0){
				nextFrameTime = now;
				paused = false;
			}
			auto ptr = (char*)nextFrame->mappedBuffer.pData;
			if(ptr){
				ptr += sequence->RowPitch()*4-sequence->DataOffset();
				auto file = CreateFileA(sequence->Image(nextFrame->nextToLoad).c_str(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED,NULL);
				if (file != INVALID_HANDLE_VALUE) {
					ZeroMemory(&nextFrame->overlap,sizeof(OVERLAPPED));
					nextFrame->overlap.hEvent = nextFrame->waitEvent;
					nextFrame->file = file;
					DWORD bytesRead=0;
					DWORD totalBytesRead=0;
					DWORD totalToRead = numbytesdata;
					//SetFilePointer(file,HEADER_SIZE_BYTES,nullptr,FILE_BEGIN);
					//while(totalBytesRead<totalToRead){
						ReadFile(file,ptr+totalBytesRead,totalToRead - totalBytesRead,&bytesRead,&nextFrame->overlap);
						//totalBytesRead += bytesRead;
					//}
					nextFrame->fps = nextFps;
					nextFrame->loadTime = now;
					nextFrame->presentationTime = nextFrameTime;
					if(!readyToWait->send(nextFrame))
					{
						CloseHandle(nextFrame->file);
					}
				}
			}
		}
	});
	
	// waiter thread: waits for the transfer from disk to GPU mem to finish
	// and sends the frame num to the rate thread
	OutputDebugString( L"creating waiter thread\n" );
	m_WaiterThreadRunning = true;
	running = &m_WaiterThreadRunning;
	auto avgDecodeDuration = &m_AvgDecodeDuration;
	m_WaiterThread = std::thread([running,readyToWait,readyToRate,avgDecodeDuration]{
		auto start = HighResClock::now();
		Frame * nextFrame = nullptr;
		while(readyToWait->recv(nextFrame)){
			WaitForSingleObject(nextFrame->waitEvent,INFINITE);
			*avgDecodeDuration = std::chrono::milliseconds(std::chrono::duration_cast<std::chrono::milliseconds>(HighResClock::now() - nextFrame->loadTime).count()/(readyToWait->size()+1));
			CloseHandle(nextFrame->file);
			readyToRate->send(nextFrame);
		}
	});


	// rate thread: receives frames already in the GPU and controls
	// the playback rate, once a frame is on time to be shown it sends it
	// to the main thread
	m_RateThreadRunning = true;
	running = &m_RateThreadRunning;
	auto droppedFrames = &m_DroppedFrames;
	m_RateThread = std::thread([running,readyToRate,readyToUpload,readyToRender,fps,droppedFrames,internalRate] {
		Frame * nextFrame = nullptr;
		auto prevFps = *fps;
		auto absFps = abs(prevFps);
		uint64_t periodns;
		if(absFps!=0){
			periodns = 1000000000 / absFps;
		}else{
			periodns = 1000000000 / 60;
		}
		std::chrono::nanoseconds p(periodns);
		Timer timer(p);
		bool wasInternalRate = *internalRate;
		while(*running){
			auto useInternalRate = *internalRate;
			if(useInternalRate){
				if(!wasInternalRate){
					timer.reset();
				}
				if(!readyToRate->recv(nextFrame)){
					break;
				}
				timer.wait_next();
				auto nextFps = *fps;
				if(prevFps != nextFps){
					if(nextFrame->fps!=nextFps && ((prevFps>0 && nextFps<0) || (nextFps>0 && prevFps<0))){
						auto nextToLoad = nextFrame->nextToLoad;
						auto waitingFor = nextToLoad;
						do{
							nextFrame->nextToLoad = nextToLoad;
							if(!readyToUpload->send(nextFrame)){
								return;
							}
							nextToLoad = -1;
							if(!readyToRate->recv(nextFrame)){
								return;
							}
						}while(nextFrame->nextToLoad!=waitingFor);
					}
					prevFps = nextFps;
					absFps = abs(prevFps);
					if(absFps!=0){
						periodns = 1000000000 / absFps;
					}else{
						periodns = 1000000000 / 60;
					}
					timer.set_period(std::chrono::nanoseconds(periodns));
				}
				if(!readyToRender->send(nextFrame)){
					break;
				}
			}else{
				if(!readyToRate->recv(nextFrame) ||	!readyToRender->send(nextFrame)){
					break;
				}
			}
			wasInternalRate = useInternalRate;
		}
	});
}

DX11Player::~DX11Player(){
	m_RateThreadRunning = false;
	m_UploaderThreadRunning = false;
	m_WaiterThreadRunning = false;
	m_ReadyToUpload.close();
	m_ReadyToRate.close();
	m_ReadyToRender.close();
	m_ReadyToWait.close();
	m_NextFrameChannel.close();
	m_RateThread.join();
	m_UploaderThread.join();
	m_WaiterThread.join();
	if(m_BackBuffer){
		m_BackBuffer->Release();
	}
	if(m_RenderTargetView){
		m_RenderTargetView->Release();
	}
	if(m_TextureBack){
		m_TextureBack->Release();
	}
	for(auto srv: m_ShaderResourceViews){
		srv->Release();
	}
	for(auto t: m_CopyTextureIn){
		t->Release();
	}
	for(auto & f: m_FramePool){
		f.uploadBuffer->Release();
	}
	m_Context->Release();
	m_Device->Release();
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
	if(!m_UploaderThreadRunning) return;
	Frame * frame=nullptr;
	auto now = HighResClock::now();
	auto max_lateness = m_AvgPipelineLatency + std::chrono::milliseconds(2);
	HighResClock::duration currentLatency;
	bool late = true;
	std::vector<Frame*> receivedFrames;
	while(late && m_ReadyToRender.try_recv(frame)){
		currentLatency += now - frame->loadTime;
		receivedFrames.push_back(frame);
		late = !m_InternalRate || IsFrameLate(frame->presentationTime,abs(m_Fps),now,max_lateness);
	}
	if(!receivedFrames.empty()){
		m_AvgPipelineLatency = std::chrono::nanoseconds(std::chrono::duration_cast<std::chrono::nanoseconds>(currentLatency).count() / receivedFrames.size());
	}

	if(!receivedFrames.empty()){
		m_NextRenderFrame = *receivedFrames.back();
		m_Context->Unmap(m_NextRenderFrame.uploadBuffer, 0);

		m_Context->CopySubresourceRegion(m_CopyTextureIn[m_CurrentOutFront],0,0,0,0,m_NextRenderFrame.uploadBuffer,0,&m_CopyBox);
		
		m_Context->Map(m_NextRenderFrame.uploadBuffer,0,D3D11_MAP_WRITE,0,&m_NextRenderFrame.mappedBuffer);
		m_DroppedFrames+=int(receivedFrames.size())-1;
		//m_CurrentOutFront += 1;
		//m_CurrentOutFront %= OUTPUT_BUFFER_SIZE;

		if(m_Sequence->InputFormat() != ImageSequence::DX11_NATIVE){
			m_Context->PSSetShaderResources(0,1,&m_ShaderResourceViews[m_CurrentOutFront]);
			m_Context->Draw(6, 0);
			m_Context->Flush();

			m_Context->CopyResource(m_TextureBack, m_BackBuffer);
		} 

	}

	for(auto buffer: receivedFrames){
		buffer->nextToLoad = -1;
		m_ReadyToUpload.send(buffer);
	}
}


HANDLE DX11Player::GetSharedHandle(){
	if(!m_UploaderThreadRunning) return nullptr;
	if(m_Sequence->InputFormat() == ImageSequence::DX11_NATIVE){
		return m_SharedHandles[m_CopyTextureIn[m_CurrentOutFront]];
	}else{
		return m_SharedHandles[m_TextureBack];
	}
}

std::string DX11Player::GetDirectory() const
{
	if(!m_UploaderThreadRunning) return "";
	return m_Sequence->Directory();
}

int DX11Player::GetUploadBufferSize() const
{
	if(!m_UploaderThreadRunning) return 0;
	return m_ReadyToUpload.size();
}

int DX11Player::GetWaitBufferSize() const
{
	if(!m_UploaderThreadRunning) return 0;
	return m_ReadyToWait.size();
}

int DX11Player::GetRenderBufferSize() const
{
	if(!m_UploaderThreadRunning) return 0;
	return m_ReadyToRender.size();
}

int DX11Player::GetDroppedFrames() const
{
	return m_DroppedFrames;
}


size_t DX11Player::GetCurrentLoadFrame() const
{
	return m_CurrentFrame;
}


size_t DX11Player::GetCurrentRenderFrame() const
{
	return m_NextRenderFrame.nextToLoad;
}


void DX11Player::SetFPS(int fps){
	m_Fps = fps;
}

int DX11Player::GetAvgLoadDurationMs() const
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(m_AvgDecodeDuration).count();
}

void DX11Player::SendNextFrameToLoad(int nextFrame)
{
	m_ExternalRate = true;
	if(!m_UploaderThreadRunning) return;
	m_NextFrameChannel.send(nextFrame);
}


void DX11Player::SetInternalRate(int enabled)
{
	m_InternalRate = enabled;
	if(enabled && m_ExternalRate)
	{
		m_ExternalRate = false;
		m_NextFrameChannel.send(m_CurrentFrame);
	}
}

bool DX11Player::IsReady() const{
	return m_UploaderThreadRunning;
}

extern "C"{
	NATIVE_API DX11HANDLE DX11Player_Create(const char * directory)
	{
		try{
			return new DX11Player(directory);
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

	NATIVE_API int DX11Player_GetAvgLoadDurationMs(DX11HANDLE player)
	{
		return static_cast<DX11Player*>(player)->GetAvgLoadDurationMs();
	}
	
	NATIVE_API void DX11Player_SetFPS(DX11HANDLE player, int fps)
	{
		static_cast<DX11Player*>(player)->SetFPS(fps);
	}
	
	NATIVE_API void DX11Player_SendNextFrameToLoad(DX11HANDLE player, int nextFrame)
	{
		static_cast<DX11Player*>(player)->SendNextFrameToLoad(nextFrame);
	}

	NATIVE_API void DX11Player_SetInternalRate(DX11HANDLE player, int enabled)
	{
		static_cast<DX11Player*>(player)->SetInternalRate(enabled);
	}

	NATIVE_API bool DX11Player_IsReady(DX11HANDLE player)
	{
		return static_cast<DX11Player*>(player)->IsReady();
	}

}
