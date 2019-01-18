#include "stdafx.h"
#include "Context.h"
#include "ImageFormat.h"
#include <DirectXMath.h>
#include "VShader.h"
#include "RGB_to_RGBA.h"
#include "RGB_R8_to_RGBA.h"
#include "A2R10G10B10_to_R10G10B10A2.h"
#include "PSCbYCr888_to_RGBA8888.h"
#include "PSCbYCr101010_to_R10G10B10A2.h"
#include "PSCbYCr161616_to_RGBA16161616.h"
#include "PSRGBA8888_remove_padding.h"
#include "Frame.h"
#include "Noop.h"


static DWORD NextMultiple(DWORD in, DWORD multiple){
	if (in % multiple != 0){
		return in + (multiple - in % multiple);
	}
	else{
		return in;
	}
}

Context::Context(const std::string & fileForFormat)
	:m_Device(nullptr)
	,m_Context(nullptr)
	,m_ShaderResourceView(nullptr)
	,m_BackBuffer(nullptr)
	,m_RenderTargetView(nullptr)
	,m_CopyTextureIn(nullptr)
	,m_Format(fileForFormat){

	HRESULT hr;

	// create a dx11 device
	///OutputDebugString( L"creating device\n" );
	D3D_FEATURE_LEVEL level;
	// Define the ordering of feature levels that Direct3D attempts to create.
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_1
	};

	UINT flags = D3D11_CREATE_DEVICE_SINGLETHREADED;
#ifndef NDEBUG
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	hr = D3D11CreateDevice(nullptr,
		D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		flags,
		featureLevels,
		ARRAYSIZE(featureLevels),
		D3D11_SDK_VERSION,
		&m_Device,
    &level,
		&m_Context);

	if(FAILED(hr)){
		auto lpBuf = new char[1024];
		ZeroMemory(lpBuf, sizeof(lpBuf));
		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr, hr, 0, lpBuf, 0, NULL);
		throw std::exception((std::string("Coudln't create device: ") + lpBuf).c_str());
	}
	
	// Create a texture description for the frames render texture
	m_RenderTextureDescription.MipLevels = 1;
	m_RenderTextureDescription.ArraySize = 1;
	m_RenderTextureDescription.SampleDesc.Count = 1;
	m_RenderTextureDescription.SampleDesc.Quality = 0;
	m_RenderTextureDescription.Usage = D3D11_USAGE_DEFAULT;
	m_RenderTextureDescription.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	m_RenderTextureDescription.CPUAccessFlags = 0;
	m_RenderTextureDescription.Width = m_Format.out_w;
	m_RenderTextureDescription.Height = m_Format.h;
	m_RenderTextureDescription.Format = m_Format.out_format;
	m_RenderTextureDescription.MiscFlags = D3D11_RESOURCE_MISC_SHARED & ~D3D11_RESOURCE_MISC_TEXTURECUBE;
        try {
          if ((ImageFormat::IsRGBA(m_Format.in_format) ||
               ImageFormat::IsBGRA(m_Format.in_format)) &&
              m_Format.pixel_format == ImageFormat::DX11_NATIVE) {
            m_Format.row_padding = 0;
            auto mapped_pitch = GetFrame()->GetMappedRowPitch();
            m_Format.row_padding =
                mapped_pitch / m_Format.bytes_per_pixel_in - m_Format.w;
            if (m_Format.row_padding > 0) {
              m_Format.pixel_format = ImageFormat::RGBA_PADDED;
            }
            OutputDebugStringA(
                ("Image pitch = " + std::to_string(m_Format.row_pitch) +
                 " GPU pitch = " + std::to_string(mapped_pitch) + " = " +
                 std::to_string(m_Format.row_padding) + "\n")
                    .c_str());
          } else if (m_Format.pixel_format == ImageFormat::RGB &&
                     m_Format.in_format == DXGI_FORMAT_R8_UNORM) {
            m_Format.row_padding = 0;
            auto mapped_pitch = GetFrame()->GetMappedRowPitch();
            m_Format.row_padding = (mapped_pitch - m_Format.row_pitch);
            OutputDebugStringA(
                ("Image pitch = " + std::to_string(m_Format.row_pitch) +
                 " GPU pitch = " + std::to_string(mapped_pitch) + " = " +
                 std::to_string(m_Format.row_padding) + "\n")
                    .c_str());
          } else if (m_Format.pixel_format == ImageFormat::RGB) {
            m_Format.row_padding = 0;
            auto mapped_pitch = GetFrame()->GetMappedRowPitch();
            m_Format.row_padding =
                mapped_pitch / m_Format.bytes_per_pixel_in - m_Format.w;
            OutputDebugStringA(("Image w = " + std::to_string(m_Format.w) +
                                " GPU pitch = " + std::to_string(mapped_pitch) +
                                " = " + std::to_string(m_Format.row_padding) +
                                "\n")
                                   .c_str());
          } else if (m_Format.pixel_format == ImageFormat::ARGB) {
            m_Format.row_padding = 0;
            auto mapped_pitch = GetFrame()->GetMappedRowPitch();
            m_Format.row_padding = (mapped_pitch - m_Format.row_pitch) /
                                   m_Format.bytes_per_pixel_in;
            OutputDebugStringA(
                ("Image pitch = " + std::to_string(m_Format.row_pitch) +
                 " GPU pitch = " + std::to_string(mapped_pitch) + " = " +
                 std::to_string(m_Format.row_padding) + "\n")
                    .c_str());
          } else if (m_Format.IsBC()) {
            auto mapped_pitch = GetFrame()->GetMappedRowPitch();
            if (mapped_pitch != m_Format.row_pitch) {
              m_Format.copytype = ImageFormat::DiskToRam;
            }
          }
        } catch (std::exception &e) {
          throw std::exception(
              (std::string("Trying to get frame mapped row pitch:\n") +
               e.what() + "\n")
                  .c_str());
        }

        // box to copy upload buffer to texture skipping 4 first rows
        // to skip header
        if (m_Format.copytype == ImageFormat::DiskToGpu) {
          m_CopyBox.back = 1;
          m_CopyBox.bottom = m_Format.h + 4;
          m_CopyBox.front = 0;
          m_CopyBox.left = 0;
          m_CopyBox.right = m_Format.w + m_Format.row_padding;
          m_CopyBox.top = 4;
        }

        // If the input format is not directly supported by DX11
        // create a shader to process them and output the data
        // as RGBA
        if (m_Format.in_format != m_Format.out_format || m_Format.pixel_format != ImageFormat::DX11_NATIVE)
          {
            if (m_Format.copytype == ImageFormat::DiskToRam) {
              if (m_Format.pixel_format == ImageFormat::RGBA_PADDED) {
                return;
              } else if(m_Format.pixel_format !=ImageFormat::DX11_NATIVE){
                throw std::exception("Copy to ram only supported for native types: DDS or RGBA");
              }
            }
            //OutputDebugStringA("Format not directly supported setting up colorspace conversion shader");

            // create the copy textures, used to copy from the upload
            // buffers to pass to the colorspace conversion shader
            //OutputDebugString( L"creating output textures\n" );
            D3D11_TEXTURE2D_DESC textureDescriptionCopy = m_RenderTextureDescription;
            textureDescriptionCopy.Width = m_Format.w + m_Format.row_padding;
            textureDescriptionCopy.Height = m_Format.h;
            textureDescriptionCopy.Format = m_Format.in_format;
            textureDescriptionCopy.MiscFlags = 0;
            hr = m_Device->CreateTexture2D(&textureDescriptionCopy, nullptr, &m_CopyTextureIn);
            if (FAILED(hr)){
              throw std::exception("Coudln't create copy texture\n");
            }

            // figure out the conversion shaders from the 
            // image sequence input format and depth
            const BYTE * pixelShaderSrc = nullptr;
            size_t sizePixelShaderSrc;
            bool useSampler = true;

            switch (m_Format.pixel_format){
            case ImageFormat::ARGB:
              if (m_Format.depth == 10){
                pixelShaderSrc = A2R10G10B10_to_R10G10B10A2;
                sizePixelShaderSrc = sizeof(A2R10G10B10_to_R10G10B10A2);
                useSampler = false;
              }else{
                throw std::exception("ARGB conversion only supported for 10bits");
              }
              break;

            case ImageFormat::RGB:
              if (m_Format.in_format == DXGI_FORMAT_R8_UNORM){
                pixelShaderSrc = RGB_R8_to_RGBA;
                sizePixelShaderSrc = sizeof(RGB_R8_to_RGBA);
              } else {
                pixelShaderSrc = RGB_to_RGBA;
                sizePixelShaderSrc = sizeof(RGB_to_RGBA);
              }
              break;

            case ImageFormat::RGBA_PADDED:
              pixelShaderSrc = PSRGBA8888_remove_padding;
              sizePixelShaderSrc = sizeof(PSRGBA8888_remove_padding);
              break;

            case ImageFormat::DX11_NATIVE:
              pixelShaderSrc = Noop;
              sizePixelShaderSrc = sizeof(Noop);
              break;

            case ImageFormat::CbYCr:
              switch(m_Format.depth){
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
            }

            // Set the viewport to the size of the image
            D3D11_VIEWPORT viewport = {0.0f, 0.0f, float(m_Format.out_w), float(m_Format.h), 0.0f, 1.0f};
            m_Context->RSSetViewports(1,&viewport); 
            //OutputDebugString( L"Set viewport" );

            // Create Pixel and Vertex shaders
            ID3D11VertexShader *vertexShader;
            ID3D11PixelShader  *pixelShader;
            hr = m_Device->CreateVertexShader(VShader,sizeof(VShader),nullptr,&vertexShader);
            if(FAILED(hr)){
              throw std::exception("Coudln't create vertex shader\n");
            }
            //OutputDebugString( L"Created vertex shader\n" );

            hr = m_Device->CreatePixelShader(pixelShaderSrc,sizePixelShaderSrc,nullptr,&pixelShader);
            if(FAILED(hr)){
              throw std::exception("Coudln't create pixel shader\n");
            }
            //OutputDebugString( L"Created pixel shader\n" );

            m_Context->VSSetShader(vertexShader,nullptr,0);
            m_Context->PSSetShader(pixelShader,nullptr,0);
            //OutputDebugString( L"Created vertex and pixel shaders\n" );

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
            //OutputDebugString( L"Created vertex buffer\n" );

            D3D11_INPUT_ELEMENT_DESC layout[] = {
                {"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
                {"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(DirectX::XMFLOAT3), D3D11_INPUT_PER_VERTEX_DATA, 0},
            };

            ID3D11InputLayout * vertexLayout;
            hr = m_Device->CreateInputLayout(layout, 2, VShader, sizeof(VShader), &vertexLayout);
            if(FAILED(hr)){
              throw std::exception("Coudln't create input layout\n");
            }
            //OutputDebugString( L"Created vertex layout\n" );
            m_Context->IASetInputLayout(vertexLayout);
            //OutputDebugString( L"Set vertex layout\n" );
            vertexLayout->Release();
            UINT stride = sizeof(Vertex);
            UINT offset = 0;
            m_Context->IASetVertexBuffers(0,1,&vertexBuffer,&stride,&offset);
            m_Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            //OutputDebugString( L"Created vertex buffer and topology\n" );
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
              //OutputDebugString( L"Created sampler state\n" );
              m_Context->PSSetSamplers(0,1,&samplerState);
              //OutputDebugString( L"Set sampler state\n" );
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
                float RequiresSwap;
                float RowPadding;
                uint32_t Remainder;
              };

            PS_CONSTANT_BUFFER constantData;
            constantData.InputWidth = (float)m_Format.w;
            constantData.InputHeight = (float)m_Format.h;
            constantData.OutputWidth = (float)m_Format.out_w;
            constantData.OutputHeight = (float)m_Format.h;
            constantData.OnePixel = (float)(1.0 / (float)m_Format.out_w);
            constantData.YOrigin = (float)(m_Format.vflip?1.0:0.0);
            constantData.YCoordinateSign = (float)(m_Format.vflip?-1.0:1.0);
            constantData.RequiresSwap = m_Format.byteswap?1.0:0.0;
            constantData.RowPadding = (float)m_Format.row_padding;
            constantData.Remainder = (float)m_Format.remainder;

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
            //OutputDebugString( L"Created constants buffer\n" );
            m_Context->PSSetConstantBuffers(0,1,&varsBuffer);
            //OutputDebugString( L"Set constants buffer\n" );
            varsBuffer->Release();
            //OutputDebugString( L"Created shader resource views\n" );
            // Create a backbuffer texture to render to
            D3D11_TEXTURE2D_DESC backBufferTexDescription;
            backBufferTexDescription.MipLevels = 1;
            backBufferTexDescription.ArraySize = 1;
            backBufferTexDescription.SampleDesc.Count = 1;
            backBufferTexDescription.SampleDesc.Quality = 0;
            backBufferTexDescription.Usage = D3D11_USAGE_DEFAULT;
            backBufferTexDescription.BindFlags = D3D11_BIND_RENDER_TARGET;
            backBufferTexDescription.CPUAccessFlags = 0;
            backBufferTexDescription.MiscFlags = 0;
            backBufferTexDescription.Width = m_Format.out_w;
            backBufferTexDescription.Height = m_Format.h;
            backBufferTexDescription.Format = m_Format.out_format;

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

            // Create shader resource views to be able to access the 
            // source textures
            hr = m_Device->CreateShaderResourceView(m_CopyTextureIn, nullptr, &m_ShaderResourceView);
            if (FAILED(hr)){
              throw std::exception("Coudln't create shader resource view\n");
            }
            m_Context->PSSetShaderResources(0,1,&m_ShaderResourceView);	
          }
        OutputDebugStringA("Done allocating context\n");
}

Context::~Context(){
	if(m_BackBuffer){
		m_BackBuffer->Release();
	}
	if(m_RenderTargetView){
		m_RenderTargetView->Release();
	}
	if (m_ShaderResourceView) {
		m_ShaderResourceView->Release();
	}
	if (m_CopyTextureIn) {
		m_CopyTextureIn->Release();
	}
	m_Context->Release();
	m_Device->Release();
}

ImageFormat Context::GetFormat() const{
	return m_Format;
}

void ReleaseFrame(Frame * frame){
	frame->Cancel();
	std::unique_lock<std::mutex> lock(frame->context->mutex);
	frame->context->m_Frames.emplace_back(frame,&ReleaseFrame);
}

std::shared_ptr<Frame> Context::GetFrame(){
	return std::shared_ptr<Frame>(new Frame(this));

	// DISABLED Caching of frames
	/*std::unique_lock<std::mutex> lock(mutex);
	if(m_Frames.empty()){
		std::shared_ptr<Frame> frame(new Frame(this), &ReleaseFrame);
		return frame;
	}else{
		auto frame = m_Frames.back();
		m_Frames.pop_back();
		return frame;
	}*/
}

ID3D11DeviceContext * Context::GetDX11Context(){
	return m_Context;
}


void Context::CopyFrameToOutTexture(Frame * frame) {
  if (m_Format.in_format != m_Format.out_format ||
      m_Format.pixel_format != ImageFormat::DX11_NATIVE) {
    if (m_Format.copytype == ImageFormat::DiskToGpu) {
      m_Context->CopySubresourceRegion(m_CopyTextureIn, 0, 0, 0, 0,
                                       frame->UploadBuffer(), 0, &m_CopyBox);
    } else {
      m_Context->UpdateSubresource(m_CopyTextureIn, 0, nullptr,
                                   frame->GetRAMBuffer(),
                                   static_cast<UINT>(m_Format.row_pitch),
                                   static_cast<UINT>(m_Format.bytes_data));
    }
    m_Context->Draw(6, 0);
    m_Context->CopyResource(frame->RenderTexture(), m_BackBuffer);
    m_Context->Flush();
  } else if (m_Format.copytype == ImageFormat::DiskToGpu) {
    // OutputDebugStringA(("copying " + frame->SourcePath() + " to render
    // texture\n").c_str());
    m_Context->CopySubresourceRegion(frame->RenderTexture(), 0, 0, 0, 0,
                                     frame->UploadBuffer(), 0, &m_CopyBox);
  } else {
    // m_Context->CopyResource(frame->RenderTexture(), frame->UploadBuffer());
    m_Context->UpdateSubresource(frame->RenderTexture(), 0, nullptr,
                                 frame->GetRAMBuffer(),
                                 static_cast<UINT>(m_Format.row_pitch),
                                 static_cast<UINT>(m_Format.bytes_data));
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
	textureUploadDescription.Width = m_Format.w + m_Format.row_padding;
	textureUploadDescription.Height = m_Format.h; 
	if (m_Format.copytype == ImageFormat::DiskToGpu) {
		textureUploadDescription.Height += 8;
	}
	textureUploadDescription.Format = m_Format.in_format;
	return m_Device->CreateTexture2D(&textureUploadDescription,nullptr,texture);
}

HRESULT Context::CreateRenderTexture(ID3D11Texture2D ** texture, uint8_t *initialData)
{
	if (initialData){
		D3D11_SUBRESOURCE_DATA data;
		data.pSysMem = initialData;
		data.SysMemPitch = m_Format.row_pitch;
		data.SysMemSlicePitch = m_Format.bytes_data;
		return m_Device->CreateTexture2D(&m_RenderTextureDescription, &data, texture);
	}
	else{
		return m_Device->CreateTexture2D(&m_RenderTextureDescription, nullptr, texture);
	}
}
