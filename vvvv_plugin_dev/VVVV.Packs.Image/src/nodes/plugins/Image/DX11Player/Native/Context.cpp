#include "stdafx.h"
#include "Context.h"
#include "ImageSequence.h"
#include <DirectXMath.h>
#include "VShader.h"
#include "RGB_to_RGBA.h"
#include "BGR_to_RGBA.h"
#include "A2R10G10B10_to_R10G10B10A2.h"
#include "PSCbYCr888_to_RGBA8888.h"
#include "PSCbYCr101010_to_R10G10B10A2.h"
#include "PSCbYCr161616_to_RGBA16161616.h"
#include "Frame.h"

static const int OUTPUT_BUFFER_SIZE = 4;

Context::Context(const Format & format)
	:m_Device(nullptr)
	,m_Context(nullptr)
	,m_CopyTextureIn(OUTPUT_BUFFER_SIZE,nullptr)
	,m_ShaderResourceViews(OUTPUT_BUFFER_SIZE,nullptr)
	,m_BackBuffer(nullptr)
	,m_RenderTargetView(nullptr)
	,m_TextureBack(nullptr)
	,m_CurrentOutFront(OUTPUT_BUFFER_SIZE/2)
	,m_Format(format){

	// box to copy upload buffer to texture skipping 4 first rows
	// to skip header
	m_CopyBox.back = 1;
	m_CopyBox.bottom = format.h + 4;
	m_CopyBox.front = 0;
	m_CopyBox.left = 0;
	m_CopyBox.right = format.w;
	m_CopyBox.top = 4;

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
	textureDescriptionCopy.Width = format.w;
	textureDescriptionCopy.Height = format.h;
	textureDescriptionCopy.Format = format.in_format;
	if(format.format == ImageSequence::DX11_NATIVE){
		textureDescriptionCopy.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
	}else{
		textureDescriptionCopy.MiscFlags = 0;
	}
	for(size_t i=0;i<m_CopyTextureIn.size();i++){
		hr = m_Device->CreateTexture2D(&textureDescriptionCopy,nullptr,&m_CopyTextureIn[i]);
		// create a shared handle for each texture
		if(format.format == ImageSequence::DX11_NATIVE){
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


	// If the input format is not directly supported by DX11
	// create a shader to process them and output the data
	// as RGBA
	if (format.format != ImageSequence::DX11_NATIVE)
	{
		OutputDebugStringA("Format not directly supported setting up colorspace conversion shader");

		// figure out the conversion shaders from the 
		// image sequence input format and depth
		const BYTE * pixelShaderSrc;
		size_t sizePixelShaderSrc;
		bool useSampler = true;

		switch(format.format){
		case ImageSequence::ARGB:
			if(format.depth == 10){
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
			switch(format.depth){
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
		D3D11_VIEWPORT viewport = {0.0f, 0.0f, float(format.out_w), float(format.h), 0.0f, 1.0f};
		m_Context->RSSetViewports(1,&viewport); 
		OutputDebugString( L"Set viewport" );

		// Create Pixel and Vertex shaders
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

		m_Context->VSSetShader(vertexShader,nullptr,0);
		m_Context->PSSetShader(pixelShader,nullptr,0);
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
			hr = m_Device->CreateSamplerState(&samplerDesc,&samplerState);
			if(FAILED(hr)){
				throw std::exception("Coudln't create sampler\n");
			}
			OutputDebugString( L"Created sampler state\n" );

			m_Context->PSSetSamplers(0,1,&samplerState);
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
		constantData.InputWidth = (float)format.w;
		constantData.InputHeight = (float)format.h;
		constantData.OutputWidth = (float)format.out_w;
		constantData.OutputHeight = (float)format.h;
		constantData.OnePixel = (float)(1.0 / (float)format.out_w);
		constantData.YOrigin = (float)(format.vflip?1.0:0.0);
		constantData.YCoordinateSign = (float)(format.vflip?-1.0:1.0);
		constantData.RequiresSwap = format.byteswap;

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

		// Create shader resource views to be able to access the 
		// source textures
		int i=0;
		for(auto & shaderResourceView: m_ShaderResourceViews){
			hr = m_Device->CreateShaderResourceView(m_CopyTextureIn[i],nullptr,&shaderResourceView);
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
		backBufferTexDescription.Width = format.out_w;
		backBufferTexDescription.Height = format.h;
		backBufferTexDescription.Format = format.out_format;

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
		
		m_Context->PSSetShaderResources(0,1,&m_ShaderResourceViews[m_CurrentOutFront]);

		textureDescriptionCopy.Width = format.out_w;
		textureDescriptionCopy.Format = format.out_format;
		textureDescriptionCopy.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
		hr = m_Device->CreateTexture2D(&textureDescriptionCopy,nullptr,&m_TextureBack);
		if(FAILED(hr)){
			throw std::exception("Coudln't create backbuffer texture\n");
		}

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

Context::~Context(){
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
	m_SharedHandles.clear();
	m_Context->Release();
	m_Device->Release();
}

Format Context::GetFormat() const{
	return m_Format;
}

void ReleaseFrame(Frame * frame){
	frame->Wait(INFINITE);
	frame->SetNextToLoad(-1);
	std::unique_lock<std::mutex> lock(frame->context->mutex);
	frame->context->m_Frames.emplace_back(frame,&ReleaseFrame);
}

std::shared_ptr<Frame> Context::GetFrame(){
	if(m_Frames.empty()){
		std::shared_ptr<Frame> frame(new Frame(this), &ReleaseFrame);
		return frame;
	}else{
		std::unique_lock<std::mutex> lock(mutex);
		auto frame = m_Frames.back();
		m_Frames.pop_back();
		frame->SetNextToLoad(-1);
		return frame;
	}
}

ID3D11DeviceContext * Context::GetDX11Context(){
	return m_Context;
}


void Context::CopyFrameToOutTexture(std::shared_ptr<Frame> frame){
	m_Context->CopySubresourceRegion(m_CopyTextureIn[m_CurrentOutFront],0,0,0,0,frame->UploadBuffer(),0,&m_CopyBox);
	if(m_Format.format != ImageSequence::DX11_NATIVE){
		m_Context->Draw(6, 0);
		m_Context->CopyResource(m_TextureBack, m_BackBuffer);
		m_Context->Flush();
	} 
}

void Context::Clear(){
	const FLOAT BLACK[4] = {0.0f,0.0f,0.0f,0.0f};
	m_Context->ClearRenderTargetView(m_RenderTargetView,BLACK);
}
	
HANDLE Context::GetSharedHandle(){
	if(m_Format.format == ImageSequence::DX11_NATIVE){
		return m_SharedHandles[m_CopyTextureIn[m_CurrentOutFront]];
	}else{
		return m_SharedHandles[m_TextureBack];
	}
}

HRESULT Context::CreateStagingTexture(ID3D11Texture2D ** texture){
	// - create the upload buffers, we upload directly to these from the disk
	// using 4 more rows than the original image to avoid the header. BC formats
	// use a 4x4 tile so we write the header at the end of the first 4 rows and 
	// then skip it when copying to the copy textures
	D3D11_TEXTURE2D_DESC textureUploadDescription;
	textureUploadDescription.MipLevels = 1;
	textureUploadDescription.ArraySize = 1;
	textureUploadDescription.SampleDesc.Count = 1;
	textureUploadDescription.SampleDesc.Quality = 0;
	textureUploadDescription.Usage = D3D11_USAGE_STAGING;
	textureUploadDescription.BindFlags = 0;
	textureUploadDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	textureUploadDescription.MiscFlags = 0;
	textureUploadDescription.Width = m_Format.w;
	textureUploadDescription.Height = m_Format.h+4;
	textureUploadDescription.Format = m_Format.in_format;
	return m_Device->CreateTexture2D(&textureUploadDescription,nullptr,texture);
}


bool operator!=(const Format & format1, const Format & format2){
	return format1.w != format2.w || format1.h != format2.h || format1.format!=format2.format || format1.in_format!=format2.in_format || format1.out_format!=format2.out_format || format1.depth!=format2.depth || format1.out_w != format2.out_w || format1.vflip!=format2.vflip || format1.byteswap!=format2.byteswap;
}