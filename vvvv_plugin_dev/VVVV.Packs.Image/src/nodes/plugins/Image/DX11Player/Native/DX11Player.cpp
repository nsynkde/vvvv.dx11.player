// Native.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "DX11Player.h"
#include "tinydir.h"
#include <comdef.h>
#include <sstream>
#include <iostream>
#include <math.h>
#include "Timer.h"
#include <memory>
#include "DDS.h"
#include <DirectXMath.h>
#include <assert.h>
#include <shlwapi.h>
#include <fstream>
#include "VShader.h"
#include "RGB_to_RGBA.h";
#include "BGR_to_RGBA.h";
#include "A2R10G10B10_to_R10G10B10A2.h"
#include "PSCbYCr888_to_RGBA8888.h"
#include "PSCbYCr101010_to_R10G10B10A2.h"
#include "PSCbYCr161616_to_RGBA16161616.h"
#include "libdpx\DPXHeader.h"
#include "libdpx\DPXStream.h"
#include "libdpx\DPX.h"
#include "libdpx\ElementReadStream.h"
#include <bitset>
#include <WinIoCtl.h>
#include <Windows.h>

#pragma comment(lib, "Kernel32.lib")

using namespace DirectX;

static const int RING_BUFFER_SIZE = 8;
static const int OUTPUT_BUFFER_SIZE = 4;

#pragma pack(push, r1, 1)
struct TGA_HEADER{
   char  idlength;
   char  colourmaptype;
   char  datatypecode;
   short int colourmaporigin;
   short int colourmaplength;
   char  colourmapdepth;
   short int x_origin;
   short int y_origin;
   short width;
   short height;
   char  bitsperpixel;
   char  imagedescriptor;
};
#pragma pack(pop, r1)

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
        std::cerr << "Error\n";
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
        std::cerr << "Error\n";
        return -1;
    }

	return diskAlignment.BytesPerLogicalSector;
}

size_t NextMultiple(size_t in, size_t multiple){
	return in + (multiple - in % multiple);
}

DX11Player::DX11Player(ID3D11Device * device, const std::string & directory)
	:m_Directory(directory)
	,m_Device(device)
	,m_Context(nullptr)
	,m_CopyTextureIn(OUTPUT_BUFFER_SIZE,nullptr)
	,m_ShaderResourceViews(OUTPUT_BUFFER_SIZE,nullptr)
	,m_BackBuffer(nullptr)
	,m_RenderTargetView(nullptr)
	,m_TextureBack(nullptr)
	,m_UploaderThreadRunning(false)
	,m_WaiterThreadRunning(false)
	,m_RateThreadRunning(false)
	,m_CurrentFrame(0)
	,m_ExternalRate(false)
	,m_InternalRate(true)
	,m_CurrentOutFront(OUTPUT_BUFFER_SIZE/2)
	,m_DroppedFrames(0)
	,m_Fps(60)
	,m_AvgDecodeDuration(0)
	,m_AvgPipelineLatency(std::chrono::milliseconds(1000/m_Fps * 2))
	,m_DirectCopy(true)
{
	HRESULT hr;

	// list all files in folder
	// TODO: filter by extension
	std::stringstream str;
	str << "trying to open folder " <<  directory << std::endl;
	OutputDebugStringA( str.str().c_str());
	tinydir_dir dir;
	tinydir_open_sorted(&dir, directory.c_str());
	str << "reading folder " <<  directory <<  " with " << dir.n_files << " files" << std::endl;
	OutputDebugStringA( str.str().c_str());
	for (size_t i = 0; i < dir.n_files; i++)
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

	auto cextension = PathFindExtensionA(m_ImageFiles[0].c_str());
	std::string extension = cextension?cextension:"";
	std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
	size_t offset = 0;
	size_t numbytesdata = 0;
	size_t rowpitch = 0;
	DXGI_FORMAT format;
	const BYTE * pixelShaderSrc;
	size_t sizePixelShaderSrc;
	DXGI_FORMAT outformat;
	bool useSampler = true;
	bool requiresSwap = false;
	bool requiresVFlip = false;
	if(extension == ".dds"){
		// parse first image header and assume all have the same format
		TexMetadata mdata;
		GetMetadataFromDDSFile(m_ImageFiles[0].c_str(),DDS_FLAGS_NONE, mdata);
		m_InputWidth = mdata.width;
		m_Width = mdata.width;
		m_Height = mdata.height;
		format = mdata.format;
		outformat = format;
		offset = mdata.bytesHeader;
		numbytesdata = mdata.bytesData;
		rowpitch = mdata.rowPitch;
		m_DirectCopy = true;
	}else if(extension == ".tga"){
		TGA_HEADER header;
		std::fstream tgafile(m_ImageFiles[0], std::fstream::in | std::fstream::binary);
		tgafile.read((char*)&header, sizeof(TGA_HEADER));
		tgafile.close();
		if(header.datatypecode!=2){
			std::stringstream str;
			str << "tga format " << (int)header.datatypecode << " not suported, only RGB(A) truecolor supported\n";
			throw std::exception(str.str().c_str());
		}

		std::stringstream str;
		str << "TGA with x,y origin: " << header.x_origin << ", " << header.y_origin << " descriptor " << (int)header.imagedescriptor << std::endl;
		OutputDebugStringA(str.str().c_str());
		requiresVFlip = !(header.imagedescriptor & 32);
		auto alphaDepth = header.imagedescriptor & 15;
		m_Width = header.width;
		m_Height = header.height;
		offset = sizeof(TGA_HEADER) + header.idlength;
		switch(alphaDepth){
			case 0:
				format = DXGI_FORMAT_R8G8B8A8_UNORM;
				outformat = format;
				m_InputWidth = header.width*3/4;
				rowpitch = header.width * 3;
				m_DirectCopy = false;
				pixelShaderSrc = BGR_to_RGBA;
				sizePixelShaderSrc = sizeof(BGR_to_RGBA);
			break;
			case 8:
				format = DXGI_FORMAT_B8G8R8A8_UNORM;
				outformat = format;
				m_InputWidth = header.width;
				rowpitch = header.width;
				m_DirectCopy = true;
			break;
			default:{	
				std::stringstream str;
				str << "tga format with alpha depth " << alphaDepth << " not suported, only RGB(A) truecolor supported\n";
				throw std::exception(str.str().c_str());
			}
		}
		numbytesdata = rowpitch * header.height;
	}else if(extension == ".dpx"){
		dpx::Header header;
		InStream stream;
		stream.Open(m_ImageFiles[0].c_str());
		header.Read(&stream);
		m_Width = header.pixelsPerLine;
		m_Height = header.linesPerElement * header.numberOfElements;
		auto dpx_descriptor = header.ImageDescriptor(0);
		std::stringstream str;
		str << "dpx with " << header.numberOfElements << " elements and format " << dpx_descriptor << " " << (int)header.BitDepth(0) << "bits packed with " << header.ImagePacking(0) << std::endl;
		str << " signed: " << header.DataSign(0)  << std::endl;
		str << " colorimetric: " << header.Colorimetric(0)  << std::endl;
		str << " low data: " << header.LowData(0)  << std::endl;
		str << " low quantity: " << header.LowQuantity(0)  << std::endl;
		str << " high data: " << header.HighData(0)  << std::endl;
		str << " high quantity: " << header.HighQuantity(0)  << std::endl;
		str << " endian swap? " << header.RequiresByteSwap() << std::endl;
		str << " white " << header.WhiteLevel() << std::endl;
		str << " black " << header.BlackLevel() << std::endl;
		str << " black gain " << header.BlackGain() << std::endl;
		str << " gamma " << header.Gamma() << std::endl;
		requiresSwap = header.RequiresByteSwap();
		str << " first 8 bytes: "  << std::endl;
		uint64_t buffer;
		dpx::ElementReadStream element(&stream);
		element.Read(header,0,0,&buffer,8);
		const char * data = (const char*) &buffer;
		for(int i=0;i<8;i++){
			str << std::bitset<8>(data[i]) << std::endl;
		}

		OutputDebugStringA(str.str().c_str());
		switch(dpx_descriptor){
		case dpx::Descriptor::kABGR:
			switch(header.BitDepth(0)){
			case 8:
				format = DXGI_FORMAT_R8G8B8A8_UNORM;
				rowpitch = m_Width * 4;
				break;
			case 10:
				format = DXGI_FORMAT_R10G10B10A2_UNORM;
				rowpitch = m_Width * 4;
				break;
			case 16:
				format = DXGI_FORMAT_R16G16B16A16_UNORM;
				rowpitch = m_Width * 4 * 2;
				break;
			case 32:
				format = DXGI_FORMAT_R32G32B32A32_FLOAT;
				rowpitch = m_Width * 4 * 4;
				break;
			}
			m_InputWidth = header.pixelsPerLine;
			m_DirectCopy = false;
			outformat = format;
			// TODO: ARGB -> RGBA
			pixelShaderSrc = RGB_to_RGBA;
			sizePixelShaderSrc = sizeof(RGB_to_RGBA);
			break;
		case dpx::Descriptor::kAlpha:
			switch(header.BitDepth(0)){
			case 8:
				format = DXGI_FORMAT_A8_UNORM;
				break;
			case 10:
			default:
				throw std::exception("Alpha only 10bits not supported");
				break;
			}
			m_DirectCopy = true;
			rowpitch = m_Width;
			outformat = format;
			m_InputWidth = header.pixelsPerLine;
			break;
		case dpx::Descriptor::kBlue:
		case dpx::Descriptor::kRed:
		case dpx::Descriptor::kGreen:
		case dpx::Descriptor::kLuma:
		case dpx::Descriptor::kDepth:
			switch(header.BitDepth(0)){
			case 8:
				format = DXGI_FORMAT_R8_UNORM;
				break;
			case 10:
				throw std::exception("One channel only 10bits not supported");
				break;
			case 16:
				format = DXGI_FORMAT_R16_UNORM;
				break;
			case 32:
				format = DXGI_FORMAT_R32_FLOAT;
				break;
			}
			m_InputWidth = header.pixelsPerLine;
			m_DirectCopy = true;
			outformat = format;
			rowpitch = m_Width;
			break;
		case dpx::Descriptor::kRGB:
			switch(header.BitDepth(0)){
			case 8:
				format = DXGI_FORMAT_R8G8B8A8_UNORM;
				outformat = format;
				rowpitch = m_Width * 3;
				m_DirectCopy = false;
				pixelShaderSrc = RGB_to_RGBA;
				sizePixelShaderSrc = sizeof(RGB_to_RGBA);
				m_InputWidth = header.pixelsPerLine*3/4;
				break;
			case 10:
				if(header.ImagePacking(0)==1){
					format = DXGI_FORMAT_R32_UINT;
					outformat = DXGI_FORMAT_R10G10B10A2_UNORM;
					rowpitch = m_Width * 4;
					m_DirectCopy = false;
					useSampler = false;
					m_InputWidth = header.pixelsPerLine;
					pixelShaderSrc = A2R10G10B10_to_R10G10B10A2;
					sizePixelShaderSrc = sizeof(A2R10G10B10_to_R10G10B10A2);
				}else{
					throw std::exception("RGB 10bits without packing to 10/10/10/2 not supported");
				}
				break;
			case 16:
				format = DXGI_FORMAT_R16G16B16A16_UNORM;
				outformat = format;
				rowpitch = m_Width * 3 * 2;
				m_DirectCopy = false;
				pixelShaderSrc = RGB_to_RGBA;
				sizePixelShaderSrc = sizeof(RGB_to_RGBA);
				m_InputWidth = header.pixelsPerLine*3/4;
				break;
			case 32:
				format = DXGI_FORMAT_R32G32B32A32_FLOAT;
				outformat = format;
				rowpitch = m_Width * 3 * 4;
				m_DirectCopy = false;
				pixelShaderSrc = RGB_to_RGBA;
				sizePixelShaderSrc = sizeof(RGB_to_RGBA);
				m_InputWidth = header.pixelsPerLine*3/4;
				break;
			}
			break;
		case dpx::Descriptor::kRGBA:
			switch(header.BitDepth(0)){
			case 8:
				format = DXGI_FORMAT_R8G8B8A8_UNORM;
				rowpitch = m_Width * 4;
				break;
			case 10:
				format = DXGI_FORMAT_R10G10B10A2_UNORM;
				rowpitch = m_Width * 4;
				break;
			case 16:
				format = DXGI_FORMAT_R16G16B16A16_UNORM;
				rowpitch = m_Width * 4 * 2;
				break;
			case 32:
				format = DXGI_FORMAT_R32G32B32A32_FLOAT;
				rowpitch = m_Width * 4 * 4;
				break;
			}
			m_InputWidth = header.pixelsPerLine;
			outformat = format;
			m_DirectCopy = true;
			break;
		case dpx::Descriptor::kCbYCr:
			switch(header.BitDepth(0)){
			case 8:
				format = DXGI_FORMAT_R8G8B8A8_UNORM;
				outformat = format;
				rowpitch = m_Width * 3;
				m_DirectCopy = false;
				pixelShaderSrc = PSCbYCr888_to_RGBA8888;
				sizePixelShaderSrc = sizeof(PSCbYCr888_to_RGBA8888);
				m_InputWidth = header.pixelsPerLine*3/4;
				break;
			case 10:
				format = DXGI_FORMAT_R32_UINT;
				outformat = DXGI_FORMAT_R10G10B10A2_UNORM;
				rowpitch = m_Width * 4;
				m_DirectCopy = false;
				useSampler = false;
				pixelShaderSrc = PSCbYCr101010_to_R10G10B10A2;
				sizePixelShaderSrc = sizeof(PSCbYCr101010_to_R10G10B10A2);
				m_InputWidth = header.pixelsPerLine;
				break;
			case 16:
				format = DXGI_FORMAT_R16G16B16A16_UNORM;
				outformat = DXGI_FORMAT_R16G16B16A16_UNORM;
				rowpitch = m_Width * 3 * 2;
				m_DirectCopy = false;
				useSampler = true;
				pixelShaderSrc = PSCbYCr161616_to_RGBA16161616;
				sizePixelShaderSrc = sizeof(PSCbYCr161616_to_RGBA16161616);
				m_InputWidth = header.pixelsPerLine*3/4;
				break;
			default:
				str << " bitdepth not supported" << std::endl;
				throw std::exception(str.str().c_str());
				break;
			}
			break;
		case dpx::Descriptor::kCbYACrYA:
		case dpx::Descriptor::kCbYCrA:
		case dpx::Descriptor::kCbYCrY:
		default:
			str << " not supported" << std::endl;
			throw std::exception(str.str().c_str());
			break;

		}
		offset = header.imageOffset;
		numbytesdata = rowpitch * m_Height;
	}else{
		throw std::exception(("format " + extension + " not suported").c_str());
	}
		
	//size_t numbytesfile = numbytesdata + offset;
	std::stringstream ss;
	ss << "loading " << m_ImageFiles.size() << " images from " << directory << " " << m_Width << "x" << m_Height << " " << format << " with " << numbytesdata << " bytes per file and " << offset << " offset, rowpitch: " << rowpitch << std::endl;
	OutputDebugStringA( ss.str().c_str() ); 
	
	// Add enough bytes to read the header + data but we need to read
	// a multiple of the physical sector size since we are reading 
	// directly from disk with no buffering
	auto sectorSize = SectorSize('d');
	numbytesdata = NextMultiple(numbytesdata+offset,sectorSize);

	// box to copy upload buffer to texture skipping 4 first rows
	// to skip header
	m_CopyBox.back = 1;
	m_CopyBox.bottom = m_Height + 4;
	m_CopyBox.front = 0;
	m_CopyBox.left = 0;
	m_CopyBox.right = m_InputWidth;
	m_CopyBox.top = 4;


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
	

	// create the copy textures, used to copy from the upload
	// buffers and shared with the application or plugin
	OutputDebugString( L"creating output textures\n" );
	D3D11_TEXTURE2D_DESC textureDescriptionCopy;
	textureDescriptionCopy.MipLevels = 1;
	textureDescriptionCopy.ArraySize = 1;
	textureDescriptionCopy.SampleDesc.Count = 1;
	textureDescriptionCopy.SampleDesc.Quality = 0;
	textureDescriptionCopy.Usage = D3D11_USAGE_DEFAULT;
	textureDescriptionCopy.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	textureDescriptionCopy.CPUAccessFlags = 0;
	textureDescriptionCopy.Width = m_InputWidth;
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
				throw std::exception("Coudln't query interface\n");
			}
			hr = pTempResource->GetSharedHandle(&sharedHandle);
			if(FAILED(hr)){
				throw std::exception("Coudln't get shared handle\n");
			}
			m_SharedHandles[m_CopyTextureIn[i]] = sharedHandle;
			pTempResource->Release();
		}
	}

	
	// init the ring buffer:
	// - map all the upload buffers and send them to the uploader thread.
	// - create the upload buffers, we upload directly to these from the disk
	// using 4 more rows than the original image to avoid the header. BC formats
	// use a 4x4 tile so we write the header at the end of the first 4 rows and 
	// then skip it when copying to the copy textures
	for(int i=0;i<RING_BUFFER_SIZE;i++){
		Frame nextFrame;
		nextFrame.idx = i;
		nextFrame.waitEvent = CreateEventW(0,false,false,0);
		hr = CreateStagingTexture(m_InputWidth,m_Height+4,format,&nextFrame.uploadBuffer);
		if(FAILED(hr)){
			_com_error error(hr);
			auto msg = error.ErrorMessage();

			auto lpBuf = new char[1024];
			ZeroMemory(lpBuf,sizeof(lpBuf));
			FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), lpBuf, 0, NULL);
			throw std::exception((std::string("Coudln't create staging texture") + lpBuf).c_str());
		}
		m_Context->Map(nextFrame.uploadBuffer,0,D3D11_MAP_WRITE,0,&nextFrame.mappedBuffer);
		m_ReadyToUpload.send(nextFrame);
	}

	// Some formats (TGA and DPX by now) need more processing
	// create a shader to process them and output the data
	// as RGBA
    if (!m_DirectCopy)
    {
		D3D11_VIEWPORT viewport = {0.0f, 0.0f, float(m_Width), float(m_Height), 0.0f, 1.0f};
		m_Context->RSSetViewports(1,&viewport); 
		OutputDebugString( L"Set viewport" );
		ID3D11VertexShader * vertexShader;

        ID3D11PixelShader * pixelShader;
		hr = m_Device->CreateVertexShader(VShader,sizeof(VShader),nullptr,&vertexShader);
		if(FAILED(hr)){
			throw std::exception("Coudln't create vertex shader\n");
		}
		OutputDebugString( L"Created vertex shader\n" );

		hr = m_Device->CreatePixelShader(pixelShaderSrc,sizePixelShaderSrc,nullptr,&pixelShader);
		if(FAILED(hr)){
			throw std::exception("Coudln't create pixel shader\n");
		}
		OutputDebugString( L"Created pixel shader\n" );

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
		hr = m_Device->CreateBuffer(&vertexBufferDescription,&bufferData,&vertexBuffer);
		if(FAILED(hr)){
			throw std::exception("Coudln't create vertex buffer\n");
		}
		OutputDebugString( L"Created vertex buffer\n" );

		D3D11_INPUT_ELEMENT_DESC layout[] = {
			{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(DirectX::XMFLOAT3), D3D11_INPUT_PER_VERTEX_DATA, 0},
		};

		ID3D11InputLayout * vertexLayout;
		hr = m_Device->CreateInputLayout(layout, 2, VShader, sizeof(VShader), &vertexLayout);
		if(FAILED(hr)){
			throw std::exception("Coudln't create input layout\n");
		}
		OutputDebugString( L"Created vertex layout\n" );

		m_Context->IASetInputLayout(vertexLayout);
		OutputDebugString( L"Set vertex layout\n" );
		vertexLayout->Release();

		UINT stride = sizeof(Vertex);
		UINT offset = 0;
		m_Context->IASetVertexBuffers(0,1,&vertexBuffer,&stride,&offset);
		m_Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		OutputDebugString( L"Created vertex buffer and topology\n" );
		vertexBuffer->Release();

		m_Context->VSSetShader(vertexShader,nullptr,0);
		m_Context->PSSetShader(pixelShader,nullptr,0);
		OutputDebugString( L"Created vertex and pixel shaders\n" );

		vertexShader->Release();
		pixelShader->Release();

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
			hr = m_Device->CreateSamplerState(&samplerDesc,&samplerState);
			if(FAILED(hr)){
				throw std::exception("Coudln't create sampler\n");
			}
			OutputDebugString( L"Created sampler state\n" );

			m_Context->PSSetSamplers(0,1,&samplerState);
			OutputDebugString( L"Set sampler state\n" );
			samplerState->Release();
		}


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
		constantData.InputWidth = (float)m_InputWidth;
		constantData.InputHeight = (float)m_Height;
		constantData.OutputWidth = (float)m_Width;
		constantData.OutputHeight = (float)m_Height;
		constantData.OnePixel = 1.0 / (float)m_Width;
		constantData.YOrigin = requiresVFlip?1.0:0.0;
		constantData.YCoordinateSign = requiresVFlip?-1.0:1.0;
		constantData.RequiresSwap = requiresSwap;

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
		hr = m_Device->CreateBuffer(&shaderVarsDescription,&bufferData,&varsBuffer);
		if(FAILED(hr)){
			throw std::exception("Coudln't create constant buffer\n");
		}
		OutputDebugString( L"Created constants buffer\n" );
        
		m_Context->PSSetConstantBuffers(0,1,&varsBuffer);
		OutputDebugString( L"Set constants buffer\n" );
		varsBuffer->Release();

		int i=0;
		for(auto & shaderResourceView: m_ShaderResourceViews){
			hr = m_Device->CreateShaderResourceView(m_CopyTextureIn[i],nullptr,&shaderResourceView);
			if(FAILED(hr)){
				throw std::exception("Coudln't create shader resource view\n");
			}
			i++;
		}
		OutputDebugString( L"Created shader resource views\n" );

		

		auto backBufferTexDescription = textureDescriptionCopy;
		backBufferTexDescription.MipLevels = 1;
		backBufferTexDescription.ArraySize = 1;
		backBufferTexDescription.SampleDesc.Count = 1;
		backBufferTexDescription.SampleDesc.Quality = 0;
		backBufferTexDescription.Usage = D3D11_USAGE_DEFAULT;
		backBufferTexDescription.BindFlags = D3D11_BIND_RENDER_TARGET;
		backBufferTexDescription.CPUAccessFlags = 0;
		backBufferTexDescription.MiscFlags = 0;
		backBufferTexDescription.Width = m_Width;
		backBufferTexDescription.Height = m_Height;
		backBufferTexDescription.Format = outformat;

		D3D11_RENDER_TARGET_VIEW_DESC viewDesc;
		ZeroMemory(&viewDesc,sizeof(D3D11_RENDER_TARGET_VIEW_DESC));
		viewDesc.Format = backBufferTexDescription.Format;
		viewDesc.Texture2D.MipSlice = 0;
		viewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

		hr = m_Device->CreateTexture2D(&backBufferTexDescription,nullptr,&m_BackBuffer);
		if(FAILED(hr)){
			throw std::exception("Coudln't create back buffer texture\n");
		}
		hr = m_Device->CreateRenderTargetView(m_BackBuffer,&viewDesc,&m_RenderTargetView);
		if(FAILED(hr)){
			throw std::exception("Coudln't create render target view\n");
		}
		m_Context->OMSetRenderTargets(1,&m_RenderTargetView,nullptr);


		textureDescriptionCopy.Width = m_Width;
		textureDescriptionCopy.Format = outformat;
		hr = m_Device->CreateTexture2D(&textureDescriptionCopy,nullptr,&m_TextureBack);
		if(FAILED(hr)){
			throw std::exception("Coudln't create staging texture\n");
		}
		if(!device){
			// if using our own device, create a shared handle for each texture
			OutputDebugString( L"getting shared texture handle\n" );
			IDXGIResource* pTempResource(NULL);
			HANDLE sharedHandle;
			hr = m_TextureBack->QueryInterface( __uuidof(IDXGIResource), (void**)&pTempResource );
			if(FAILED(hr)){
				throw std::exception("Coudln't query interface\n");
			}
			hr = pTempResource->GetSharedHandle(&sharedHandle);
			if(FAILED(hr)){
				throw std::exception("Coudln't get shared handle\n");
			}
			m_SharedHandles[m_TextureBack] = sharedHandle;
			pTempResource->Release();
		} 

		
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
	auto fps = &m_Fps;
	auto externalRate = &m_ExternalRate;
	auto nextFrameChannel = &m_NextFrameChannel;
	auto internalRate = &m_InternalRate;
	m_UploaderThread = std::thread([numbytesdata,rowpitch,offset,running,currentFrame,imageFiles,readyToUpload,readyToWait,fps,externalRate,nextFrameChannel,internalRate]{
		auto nextFrameTime = HighResClock::now();
		bool paused = false;
		int nextFps = *fps;
		Frame nextFrame;
		while(*running){
			nextFrame.nextToLoad = -1;
			readyToUpload->recv(nextFrame);
			auto now = HighResClock::now();
			auto absFps = abs(nextFps);
			if(*externalRate){
				nextFrameChannel->recv(*currentFrame);
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
							(*currentFrame)%=imageFiles->size();
						}else if(nextFps<0){
							(*currentFrame)-=1;
							(*currentFrame)%=imageFiles->size();
						}
					}while(nextFrameTime<now);
				}else{
					paused = true;
				}
			}
			*currentFrame%=imageFiles->size();
			nextFps = *fps;
			if(nextFrame.nextToLoad==-1){
				nextFrame.nextToLoad = *currentFrame;
			}else{
				nextFrame.nextToLoad%=imageFiles->size();
				*currentFrame = nextFrame.nextToLoad;
			}
			if(paused && nextFps!=0){
				nextFrameTime = now;
				paused = false;
			}
			auto ptr = (char*)nextFrame.mappedBuffer.pData;
			if(ptr){
				ptr += rowpitch*4-offset;
				auto file = CreateFileA(imageFiles->at(nextFrame.nextToLoad).c_str(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED,NULL);
				if (file != INVALID_HANDLE_VALUE) {
					ZeroMemory(&nextFrame.overlap,sizeof(OVERLAPPED));
					nextFrame.overlap.hEvent = nextFrame.waitEvent;
					nextFrame.file = file;
					DWORD bytesRead=0;
					DWORD totalBytesRead=0;
					DWORD totalToRead = numbytesdata;
					//SetFilePointer(file,HEADER_SIZE_BYTES,nullptr,FILE_BEGIN);
					//while(totalBytesRead<totalToRead){
						ReadFile(file,ptr+totalBytesRead,totalToRead - totalBytesRead,&bytesRead,&nextFrame.overlap);
						//totalBytesRead += bytesRead;
					//}
					nextFrame.fps = nextFps;
					nextFrame.loadTime = now;
					nextFrame.presentationTime = nextFrameTime;
					readyToWait->send(nextFrame);
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
		Frame nextFrame;
		while(*running){
			std::vector<Frame> receivedFrames;
			std::vector<HANDLE> events;
			readyToWait->recv(nextFrame);
			WaitForSingleObject(nextFrame.waitEvent,INFINITE);
			*avgDecodeDuration = std::chrono::milliseconds(std::chrono::duration_cast<std::chrono::milliseconds>(HighResClock::now() - nextFrame.loadTime).count()/(readyToWait->size()+1));
			CloseHandle(nextFrame.file);
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
		Frame nextFrame;
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
				readyToRate->recv(nextFrame);
				timer.wait_next();
				auto nextFps = *fps;
				if(prevFps != nextFps){
					if(nextFrame.fps!=nextFps && ((prevFps>0 && nextFps<0) || (nextFps>0 && prevFps<0))){
						auto nextToLoad = nextFrame.nextToLoad;
						auto waitingFor = nextToLoad;
						do{
							nextFrame.nextToLoad = nextToLoad;
							readyToUpload->send(nextFrame);
							nextToLoad = -1;
							readyToRate->recv(nextFrame);
						}while(nextFrame.nextToLoad!=waitingFor);
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
				readyToRender->send(nextFrame);
			}else{
				readyToRate->recv(nextFrame);
				readyToRender->send(nextFrame);
			}
			wasInternalRate = useInternalRate;
		}
	});
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
	auto max_lateness = m_AvgPipelineLatency + std::chrono::milliseconds(2);
	HighResClock::duration currentLatency;
	bool late = true;
	std::vector<Frame> receivedFrames;
	while(late && m_ReadyToRender.try_recv(frame)){
		currentLatency += now - frame.loadTime;
		receivedFrames.push_back(frame);
		late = !m_InternalRate || IsFrameLate(frame.presentationTime,abs(m_Fps),now,max_lateness);
	}
	if(!receivedFrames.empty()){
		m_AvgPipelineLatency = std::chrono::nanoseconds(std::chrono::duration_cast<std::chrono::nanoseconds>(currentLatency).count() / receivedFrames.size());
	}

	if(!receivedFrames.empty()){
		m_NextRenderFrame = receivedFrames.back();
		m_Context->Unmap(m_NextRenderFrame.uploadBuffer, 0);

		m_Context->CopySubresourceRegion(m_CopyTextureIn[m_CurrentOutFront],0,0,0,0,m_NextRenderFrame.uploadBuffer,0,&m_CopyBox);
		
		m_Context->Map(m_NextRenderFrame.uploadBuffer,0,D3D11_MAP_WRITE,0,&m_NextRenderFrame.mappedBuffer);
		m_DroppedFrames+=int(receivedFrames.size())-1;
		//m_CurrentOutFront += 1;
		//m_CurrentOutFront %= OUTPUT_BUFFER_SIZE;

		if(!m_DirectCopy){
			/*D3D11_BOX box;
			box.back = 1;
			box.bottom = m_Height;
			box.front = 0;
			box.left = 0;
			box.right = m_InputWidth;
			box.top = 0;
			m_Context->CopySubresourceRegion(m_BackBuffer,0,0,0,0,m_CopyTextureIn[m_CurrentOutFront],0,&m_CopyBox);*/
			m_Context->PSSetShaderResources(0,1,&m_ShaderResourceViews[m_CurrentOutFront]);
			m_Context->Draw(6, 0);
			m_Context->Flush();

			m_Context->CopyResource(m_TextureBack, m_BackBuffer);
		} 

	}

	for(auto buffer: receivedFrames){
		buffer.nextToLoad = -1;
		m_ReadyToUpload.send(buffer);
	}
}


HANDLE DX11Player::GetSharedHandle(){
	if(m_DirectCopy){
		return m_SharedHandles[m_CopyTextureIn[m_CurrentOutFront]];
	}else{
		return m_SharedHandles[m_TextureBack];
	}
}
	
ID3D11Texture2D * DX11Player::GetTexturePointer(){
	if(m_DirectCopy){
		return m_CopyTextureIn[m_CurrentOutFront];
	}else{
		return m_TextureBack;
	}
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

}
