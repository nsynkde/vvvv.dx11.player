#include "stdafx.h"
#include "ReadTextureDX11.h"
#include "BlueVelvet4.h"
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <iostream>
#include <sstream>


static bool IsRGBA(DXGI_FORMAT format);
static bool IsRGBA32(DXGI_FORMAT format);
static bool IsBGRA(DXGI_FORMAT format);
static bool IsBGRX(DXGI_FORMAT format);
static bool IsR10G10B10A2(DXGI_FORMAT format);

#pragma comment(lib,"d3dcompiler.lib")

ReadTextureDX11::ReadTextureDX11(HANDLE tex_handle, int outFormat)
	:m_Device(nullptr)
    ,m_SharedTexture(nullptr)
	,m_ReadBackData(4)
	,m_Context(nullptr)
    ,m_CurrentBack(0)
    ,m_CurrentFront(0)
    ,m_RendererOutput(0)
    ,m_DirectCopy(false)
{

	if(tex_handle==nullptr){
		throw std::exception("shared texture not set");
	}

	D3D_FEATURE_LEVEL level;
	D3D11CreateDevice(nullptr,
		D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		D3D11_CREATE_DEVICE_SINGLETHREADED,
		nullptr,
		0,
		D3D11_SDK_VERSION,
		&m_Device,&level,
		&m_Context);

	if(m_Device==nullptr){
		throw std::exception("Couldn't create device");
	}
	if(m_Context==nullptr){
		throw std::exception("Couldn't create context");
	}
	if(level==0){
		throw std::exception("Couldn't create d3d11 level features context");
	}else{
		wchar_t buffer [100];
		swprintf(buffer,100,L"Context created with level %d\n",(int)level);
		OutputDebugString( buffer );
	}

	ID3D11Texture2D * tex;
	//void * tex = static_cast<void*>(m_SharedTexture);
	auto hr = m_Device->OpenSharedResource(tex_handle,__uuidof(ID3D11Resource),(void**)&tex);
	if(FAILED(hr)){
		throw std::exception("Coudln't open shared texture");
	}
	OutputDebugString( L"Opened aux shared texture" );
	hr = tex->QueryInterface(__uuidof(ID3D11Texture2D),(void**)&m_SharedTexture);
	if(FAILED(hr)){
		throw std::exception("Coudln't QueryInterface on shared texture");
	}
	OutputDebugString( L"QueryInterface aux shared texture" );
	tex->Release();

	std::string pixelShaderSrc = "";
    auto backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    auto backTextureFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	D3D11_TEXTURE2D_DESC sharedTexDescription;
	m_SharedTexture->GetDesc(&sharedTexDescription);
	OutputDebugString( L"Got shared texture description" );
	auto backBufferWidth = sharedTexDescription.Width;
    auto backBufferHeight = sharedTexDescription.Height;
    auto swapRG = false;

	std::stringstream str;
	str << "Shared texture: " << backBufferWidth << ", " << backBufferHeight << ": " << sharedTexDescription.Format;
	WCHAR    wstr[1024];
	MultiByteToWideChar( 0,0, str.str().c_str(), str.str().size(), wstr, str.str().size()+1);
	OutputDebugString( wstr );
	
    switch ((EMemoryFormat)outFormat)
    {
        case MEM_FMT_RGBA:
            if (!IsRGBA(sharedTexDescription.Format) && !IsBGRX(sharedTexDescription.Format))
                throw std::exception("Input texture doesn't have the correct format: RGBA8888");
            if (IsRGBA32(sharedTexDescription.Format))
            {
                m_DirectCopy = true;
            }
            else
            {
                pixelShaderSrc = "PSPassthrough";
            }
            backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
            backTextureFormat = backBufferFormat;
            break;
        case MEM_FMT_YUVS:
            if (!IsRGBA(sharedTexDescription.Format) && !IsBGRA(sharedTexDescription.Format) && !IsBGRX(sharedTexDescription.Format)){
                throw std::exception("Input texture doesn't have the correct format: RGBA");
			}
            pixelShaderSrc = "PSRGBA8888_to_YUVS";
            backBufferWidth = backBufferWidth / 2;
            backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
            backTextureFormat = backBufferFormat;
            swapRG = IsBGRA(sharedTexDescription.Format);
            break;
        default:
            throw std::exception("Unsupported output format ");
	}

	auto backBufferTexDescription = sharedTexDescription;
    backBufferTexDescription.MipLevels = 1;
    backBufferTexDescription.ArraySize = 1;
	backBufferTexDescription.SampleDesc.Count = 1;
	backBufferTexDescription.SampleDesc.Quality = 0;
	backBufferTexDescription.Usage = D3D11_USAGE_DEFAULT;
	backBufferTexDescription.BindFlags = D3D11_BIND_RENDER_TARGET;
    backBufferTexDescription.CPUAccessFlags = 0;
    backBufferTexDescription.MiscFlags = 0;
    backBufferTexDescription.Width = backBufferWidth;
    backBufferTexDescription.Height = backBufferHeight;
    backBufferTexDescription.Format = backTextureFormat;

	D3D11_RENDER_TARGET_VIEW_DESC viewDesc;
	ZeroMemory(&viewDesc,sizeof(D3D11_RENDER_TARGET_VIEW_DESC));
	viewDesc.Format = backBufferTexDescription.Format;
	viewDesc.Texture2D.MipSlice = 0;
	viewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

	m_BackBuffer.resize(4);
	m_RenderTargetView.resize(4);
    for (size_t i = 0; i < m_BackBuffer.size(); i++)
    {
        hr = m_Device->CreateTexture2D(&backBufferTexDescription,nullptr,&m_BackBuffer[i]);
		if(FAILED(hr)){
			throw std::exception("Coudln't create back buffer texture");
		}
		hr = m_Device->CreateRenderTargetView(m_BackBuffer[i],&viewDesc,&m_RenderTargetView[i]);
		if(FAILED(hr)){
			throw std::exception("Coudln't create render target view");
		}
    }
	OutputDebugString( L"Created back buffer and render target views" );
    m_Context->OMSetRenderTargets(1,&m_RenderTargetView[0],nullptr);
	
	OutputDebugString( L"Set render target" );
	D3D11_VIEWPORT viewport = {0.0f, 0.0f, float(backBufferWidth), float(backBufferHeight), 0.0f, 1.0f};
	m_Context->RSSetViewports(1,&viewport); 
	OutputDebugString( L"Set viewport" );


	auto stagingTexDescription = sharedTexDescription;
    stagingTexDescription.MipLevels = 1;
    stagingTexDescription.ArraySize = 1;
	stagingTexDescription.SampleDesc.Count = 1;
	stagingTexDescription.SampleDesc.Quality = 0;
	stagingTexDescription.Usage = D3D11_USAGE_STAGING;
    stagingTexDescription.BindFlags = 0;
    stagingTexDescription.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    stagingTexDescription.MiscFlags = 0;
    stagingTexDescription.Width = backBufferWidth;
    stagingTexDescription.Height = backBufferHeight;
    stagingTexDescription.Format = backTextureFormat;
	m_TextureBack.resize(4);
    for (int i = 0; i < m_TextureBack.size(); i++)
    {
        hr = m_Device->CreateTexture2D(&stagingTexDescription,nullptr,&m_TextureBack[i]);
		if(FAILED(hr)){
			throw std::exception("Coudln't create staging texture");
		}
    }
	OutputDebugString( L"Created staging textures" );
	
    if (!m_DirectCopy)
    {
		
		const char* swapRGStr;
        if (swapRG)
        {
            swapRGStr = "1";
        }
        else
        {
            swapRGStr = "0";
        }
		D3D_SHADER_MACRO defines[] = {
			"SWAP_RB", swapRGStr,
			nullptr, nullptr
		};
		ID3DBlob * vertexBlob = nullptr;
		ID3DBlob * pixelBlob = nullptr;
		ID3DBlob * errorsBlob = nullptr;
		UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
		LPCWSTR shaderSrc = L"Shaders/ColorSpaceConversionDX11.hlsl";
		hr = D3DCompileFromFile(shaderSrc,defines,D3D_COMPILE_STANDARD_FILE_INCLUDE,
			"VShader", "vs_5_0",
			flags, 0, &vertexBlob, &errorsBlob);
		OutputDebugString( L"Compiled vertex" );
		
		if(FAILED(hr)){
			throw std::exception("Coudln't compile vertex shader");
		}
		if(errorsBlob){
			OutputDebugString( L"Got error blob compiling vertex shader" );
			errorsBlob->Release();
			errorsBlob = nullptr;
		}

		hr = D3DCompileFromFile(shaderSrc,defines,D3D_COMPILE_STANDARD_FILE_INCLUDE,
			pixelShaderSrc.c_str(), "ps_5_0",
			flags, 0, &pixelBlob, &errorsBlob);
		OutputDebugString( L"Compiled pixel shader" );
		
		if(FAILED(hr)){
			throw std::exception("Coudln't compile pixel shader");
		}
		if(errorsBlob){
			OutputDebugString( L"Got error blob compiling pixels shader" );
			errorsBlob->Release();
			errorsBlob = nullptr;
		}
		
		ID3D11VertexShader * vertexShader;
        ID3D11PixelShader * pixelShader;
		hr = m_Device->CreateVertexShader(vertexBlob->GetBufferPointer(),vertexBlob->GetBufferSize(),nullptr,&vertexShader);
		if(FAILED(hr)){
			throw std::exception("Coudln't create vertex shader");
		}
		OutputDebugString( L"Created vertex shader" );

		hr = m_Device->CreatePixelShader(pixelBlob->GetBufferPointer(),pixelBlob->GetBufferSize(),nullptr,&pixelShader);
		if(FAILED(hr)){
			throw std::exception("Coudln't create pixel shader");
		}
		OutputDebugString( L"Created pixel shader" );

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
			throw std::exception("Coudln't create vertex buffer");
		}
		OutputDebugString( L"Created vertex buffer" );

		D3D11_INPUT_ELEMENT_DESC layout[] = {
			{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(DirectX::XMFLOAT3), D3D11_INPUT_PER_VERTEX_DATA, 0},
		};

		ID3D11InputLayout * vertexLayout;
		hr = m_Device->CreateInputLayout(layout, 2, vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(), &vertexLayout);
		if(FAILED(hr)){
			throw std::exception("Coudln't create input layout");
		}
		OutputDebugString( L"Created vertex layout" );
		vertexBlob->Release();
		pixelBlob->Release();

		m_Context->IASetInputLayout(vertexLayout);
		OutputDebugString( L"Set vertex layout" );

		UINT stride = sizeof(Vertex);
		UINT offset = 0;
		m_Context->IASetVertexBuffers(0,1,&vertexBuffer,&stride,&offset);
		m_Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		OutputDebugString( L"Created vertex buffer and topology" );

		m_Context->VSSetShader(vertexShader,nullptr,0);
		m_Context->PSSetShader(pixelShader,nullptr,0);
		OutputDebugString( L"Created vertex and pixel shaders" );

		D3D11_SAMPLER_DESC samplerDesc;
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
			throw std::exception("Coudln't create sampler");
		}
		OutputDebugString( L"Created sampler state" );

		m_Context->PSSetSamplers(0,1,&samplerState);
		OutputDebugString( L"Set sampler state" );

		_declspec(align(16))
		struct PS_CONSTANT_BUFFER
		{
			float InputWidth;
			float InputHeight;
			float OutputWidth;
			float OutputHeight;
			float OnePixel;
		};

		PS_CONSTANT_BUFFER constantData;
		constantData.InputWidth = (float)sharedTexDescription.Width;
		constantData.InputHeight = (float)sharedTexDescription.Height;
		constantData.OutputWidth = (float)backBufferWidth;
		constantData.OutputHeight = (float)backBufferHeight;
		constantData.OnePixel = (float)1.0 / (float)sharedTexDescription.Width;

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
			throw std::exception("Coudln't create constant buffer");
		}
		OutputDebugString( L"Created constants buffer" );
        
		m_Context->PSSetConstantBuffers(0,1,&varsBuffer);
		OutputDebugString( L"Set constants buffer" );


		ID3D11ShaderResourceView * shaderResourceView;
		hr = m_Device->CreateShaderResourceView(m_SharedTexture,nullptr,&shaderResourceView);
		if(FAILED(hr)){
			throw std::exception("Coudln't create shader resource view");
		}
		OutputDebugString( L"Created shader resource view" );

		m_Context->PSSetShaderResources(0,1,&shaderResourceView);
		shaderResourceView->Release();
		OutputDebugString( L"Set shader resource view" );

		for(auto readBackData: m_ReadBackData){
			memset(&readBackData,0,sizeof(D3D11_MAPPED_SUBRESOURCE));
		}
		OutputDebugString( L"zeroed read back data" );
    }
}

ReadTextureDX11::~ReadTextureDX11()
{
}

	
void * ReadTextureDX11::ReadBack()
{
	if (m_ReadBackData[m_CurrentFront].pData != nullptr)
    {
		m_Context->Unmap(m_TextureBack[m_CurrentFront], 0);
		m_CurrentBack += 1;
		m_CurrentBack %= m_TextureBack.size();
    }
    m_CurrentFront = m_CurrentBack + 2;
    m_CurrentFront %= m_TextureBack.size();



    if (m_DirectCopy)
    {
        m_Context->CopyResource(m_TextureBack[m_CurrentBack], m_SharedTexture);
    }
    else
    {
        auto renderTargetView = m_RenderTargetView[m_RendererOutput];
        m_Context->OMSetRenderTargets(1,&renderTargetView,nullptr);
        m_Context->Draw(6, 0);
        auto frontBuffer = m_BackBuffer[(m_RendererOutput + 1) % m_BackBuffer.size()];
        m_Context->CopyResource(m_TextureBack[m_CurrentBack], frontBuffer);
        m_RendererOutput++;
        m_RendererOutput %= m_BackBuffer.size();
    }

	auto hr = m_Context->Map(m_TextureBack[m_CurrentFront],0, D3D11_MAP_READ, 0, &m_ReadBackData[m_CurrentFront]);
	if(FAILED(hr) || m_ReadBackData[m_CurrentFront].pData == nullptr){
		throw std::exception("Coudln't map texture");
	}
    m_Context->Flush();
	return m_ReadBackData[m_CurrentFront].pData;
}



static bool IsRGBA(DXGI_FORMAT format)
{
    return format == DXGI_FORMAT_R16G16B16A16_FLOAT ||
        format == DXGI_FORMAT_R16G16B16A16_SINT ||
        format == DXGI_FORMAT_R16G16B16A16_SNORM ||
        format == DXGI_FORMAT_R16G16B16A16_TYPELESS ||
		format == DXGI_FORMAT_R16G16B16A16_UINT ||
        format == DXGI_FORMAT_R16G16B16A16_UNORM ||
        format == DXGI_FORMAT_R32G32B32A32_FLOAT ||
        format == DXGI_FORMAT_R32G32B32A32_SINT ||
        format == DXGI_FORMAT_R32G32B32A32_TYPELESS ||
        format == DXGI_FORMAT_R32G32B32A32_UINT ||
        format == DXGI_FORMAT_R8G8B8A8_SINT ||
        format == DXGI_FORMAT_R8G8B8A8_SNORM ||
        format == DXGI_FORMAT_R8G8B8A8_TYPELESS ||
        format == DXGI_FORMAT_R8G8B8A8_UINT ||
		format == DXGI_FORMAT_R8G8B8A8_UNORM ||
		format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
}

static bool IsRGBA32(DXGI_FORMAT format)
{
    return format == DXGI_FORMAT_R8G8B8A8_SINT ||
        format == DXGI_FORMAT_R8G8B8A8_SNORM ||
        format == DXGI_FORMAT_R8G8B8A8_TYPELESS ||
        format == DXGI_FORMAT_R8G8B8A8_UINT ||
		format == DXGI_FORMAT_R8G8B8A8_UNORM ||
        format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

}

static bool IsBGRA(DXGI_FORMAT format)
{
    return format == DXGI_FORMAT_B8G8R8A8_TYPELESS ||
		format == DXGI_FORMAT_B8G8R8A8_UNORM ||
        format == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
}

static bool IsBGRX(DXGI_FORMAT format)
{
    return format == DXGI_FORMAT_B8G8R8X8_TYPELESS ||
		format == DXGI_FORMAT_B8G8R8X8_UNORM ||
        format == DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
}

static bool IsR10G10B10A2(DXGI_FORMAT format)
{
    return format == DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM ||
        format == DXGI_FORMAT_R10G10B10A2_TYPELESS ||
		format == DXGI_FORMAT_R10G10B10A2_UINT ||
        format == DXGI_FORMAT_R10G10B10A2_UNORM;
}