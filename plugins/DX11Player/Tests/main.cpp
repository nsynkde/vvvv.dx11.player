// include the basic windows header files and the Direct3D header files
//#define _CRTDBG_MAP_ALLOC
//#include <stdlib.h>
//#include <crtdbg.h>


#include <windows.h>
#include <windowsx.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include "PShader.h"
#include "PShaderNoTex.h"
#include "VShader.h"
#include <iostream>
#include "DX11Player.h"
#include <chrono>
#include "HighResClock.h"
#include <sstream>
#include <memory>
#include <chrono>
#include "tinydir.h"

//#define VLD_FORCE_ENABLE
//#include <vld.h>

// include the Direct3D Library file
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "Native.lib")


// define the screen resolution
#define SCREEN_WIDTH  1920
#define SCREEN_HEIGHT 1200

// global declarations
IDXGISwapChain *swapchain;             // the pointer to the swap chain interface
ID3D11Device *dev;                     // the pointer to our Direct3D device interface
ID3D11DeviceContext *devcon;           // the pointer to our Direct3D device context
ID3D11RenderTargetView *backbuffer;    // the pointer to our back buffer
ID3D11InputLayout *pLayout;            // the pointer to the input layout
ID3D11VertexShader *pVS;               // the pointer to the vertex shader
ID3D11PixelShader *pPS;                // the pointer to the pixel shader
ID3D11PixelShader *pPSNoTex;                // the pointer to the pixel shader
ID3D11Buffer *pVBuffer;                // the pointer to the vertex buffer
ID3D11Buffer *pVBufferVerticalLine;                // the pointer to the vertex buffer
std::shared_ptr<DX11Player> player;
HighResClock::time_point start;
std::vector<std::string> files;
static const int bufferSize = 8;

	// a struct to define a single vertex
struct Vertex
{
	DirectX::XMFLOAT4 position;
	DirectX::XMFLOAT2 texcoord;
	Vertex(const DirectX::XMFLOAT4 &position, const DirectX::XMFLOAT2 &texcoord)
		:position(position)
		,texcoord(texcoord){}
};

// function prototypes
void InitD3D(HWND hWnd);    // sets up and initializes Direct3D
void RenderFrame(void);     // renders a single frame
void CleanD3D(void);        // closes Direct3D and releases memory
void InitGraphics(void);    // creates the shape to render
void InitPipeline(void);    // loads and prepares the shaders

// the WindowProc function prototype
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


// the entry point for any Windows program
int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{
    HWND hWnd;
    WNDCLASSEX wc;
	//_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
    ZeroMemory(&wc, sizeof(WNDCLASSEX));

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "WindowClass";

    RegisterClassEx(&wc);

    RECT wr = {0, 0, 1280, 720};
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

    hWnd = CreateWindowExA(NULL,
                          "WindowClass",
                          "Our First Direct3D Program",
                          WS_OVERLAPPEDWINDOW,
                          0,
                          0,
                          wr.right - wr.left,
                          wr.bottom - wr.top,
                          NULL,
                          NULL,
                          hInstance,
                          NULL);

    ShowWindow(hWnd, nCmdShow);

    // set up and initialize Direct3D
	//try{
		InitD3D(hWnd);
	/*}catch(std::exception & exc){
		OutputDebugStringA(exc.what());
		return 1;
	}*/

    // enter the main loop:

    MSG msg;
	ZeroMemory(&msg, sizeof(MSG));
	
	/*for(int i=0;i<7;i++){
		player->SendNextFrameToLoad(files[i]);
	}*/
	while (msg.message != WM_QUIT)
    {
        if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        RenderFrame();
	}

    CleanD3D();
	//ReportLiveObjects();
    return msg.wParam;
}


// this is the main message handler for the program
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
        case WM_DESTROY:
            {
                PostQuitMessage(0);
                return 0;
            } break;
    }

    return DefWindowProc (hWnd, message, wParam, lParam);
}


// this function initializes and prepares Direct3D for use
void InitD3D(HWND hWnd)
{
    // create a struct to hold information about the swap chain
    DXGI_SWAP_CHAIN_DESC scd;

    // clear out the struct for use
    ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));

    // fill the swap chain description struct
    scd.BufferCount = 1;                                   // one back buffer
	scd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;    // use 32-bit color
    scd.BufferDesc.Width = SCREEN_WIDTH;                   // set the back buffer width
    scd.BufferDesc.Height = SCREEN_HEIGHT;                 // set the back buffer height
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;     // how swap chain is to be used
    scd.OutputWindow = hWnd;                               // the window to be used
    scd.SampleDesc.Count = 1;                              // how many multisamples
	scd.SampleDesc.Quality = 0;
    scd.Windowed = TRUE;                                   // windowed/full-screen mode
    scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;    // allow full-screen switching
	scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    // create a device, device context and swap chain using the information in the scd struct
    D3D11CreateDeviceAndSwapChain(NULL,
                                  D3D_DRIVER_TYPE_HARDWARE,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  D3D11_SDK_VERSION,
                                  &scd,
                                  &swapchain,
                                  &dev,
                                  NULL,
                                  &devcon);


    // get the address of the back buffer
    ID3D11Texture2D *pBackBuffer;
    swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

    // use the back buffer address to create the render target
    dev->CreateRenderTargetView(pBackBuffer, NULL, &backbuffer);
    pBackBuffer->Release();

    // set the render target as the back buffer
    devcon->OMSetRenderTargets(1, &backbuffer, NULL);


    // Set the viewport
    D3D11_VIEWPORT viewport;
    ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = SCREEN_WIDTH;
    viewport.Height = SCREEN_HEIGHT;

    devcon->RSSetViewports(1, &viewport);

    InitPipeline();
    InitGraphics();

	start = HighResClock::now();
}

void UpdateVerticalLine(){	
	auto now = HighResClock::now();
	float x = fmod(std::chrono::duration_cast<std::chrono::microseconds>(now-start).count()*0.5/1000000.0,1.0)*2.0f-1.0f;
	Vertex verticalLine[] = {
		Vertex(DirectX::XMFLOAT4(x, 1.0f, 0.5f, 1.0f),DirectX::XMFLOAT2(0.0f, 0.0f)),
		Vertex(DirectX::XMFLOAT4(x+0.1f, 1.0f, 0.5f, 1.0f),DirectX::XMFLOAT2(1.0f, 0.0f)),
		Vertex(DirectX::XMFLOAT4(x, -1.0f, 0.5f, 1.0f),DirectX::XMFLOAT2(0.0f, 1.0f)),

		Vertex(DirectX::XMFLOAT4(x, -1.0f, 0.5f, 1.0f),DirectX::XMFLOAT2(0.0f, 1.0f)),
		Vertex(DirectX::XMFLOAT4(x+0.1f, 1.0f, 0.5f, 1.0f),DirectX::XMFLOAT2(1.0f, 0.0f)),
		Vertex(DirectX::XMFLOAT4(x+0.1f, -1.0f, 0.5f, 1.0f),DirectX::XMFLOAT2(1.0f, 1.0f)),
	};
	D3D11_MAPPED_SUBRESOURCE resource;
    devcon->Map(pVBufferVerticalLine, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
    memcpy(resource.pData, verticalLine, sizeof(Vertex)*6);
    devcon->Unmap(pVBufferVerticalLine, 0);
}


ID3D11Texture2D * GetTexture(int framenum){
	auto sharedHandle = player->GetSharedHandle(files[framenum%files.size()]);
	if(sharedHandle!=nullptr){
		ID3D11Texture2D * tex;
		ID3D11Texture2D * sharedTex;
		auto hr = dev->OpenSharedResource(sharedHandle,__uuidof(ID3D11Resource),(void**)&tex);
		if(FAILED(hr)){
			throw std::exception("Coudln't open shared texture");
		}
		hr = tex->QueryInterface(__uuidof(ID3D11Texture2D),(void**)&sharedTex);
		tex->Release();
		return sharedTex;
	}else{
		return nullptr;
	}
}


// this is the function used to render a single frame
void RenderFrame(void)
{
	static int nextToLoad = bufferSize;
	static int nextToRender = 0;
	static auto starttime = HighResClock::now();
	static auto wasReady = false;
	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	/*if(i==300){
		player->SetFPS(0);
	}*/
    // clear the back buffer to a deep blue
	float color[] = {0.0f, 0.0f, 0.0f, 1.0f};
    devcon->ClearRenderTargetView(backbuffer, color);
	if(player->IsReady()){
		std::vector<std::string> current_frames;
		if (nextToRender < nextToLoad) {
			current_frames.assign(files.begin() + nextToRender, files.begin() + nextToLoad);
		} else {
			current_frames.assign(files.begin() + nextToRender, files.end());
			current_frames.insert(current_frames.end(), files.begin(), files.begin() + nextToLoad);
		}
		player->SetSystemFrames(current_frames);
		if(!wasReady){
			for(int i=0;i<bufferSize;i++){
				player->SendNextFrameToLoad(files[i]);
			}
			player->Update();
			std::stringstream str;
			str << "got first frame on " << nextToRender << " at " << std::chrono::duration_cast<std::chrono::milliseconds>(HighResClock::now() - starttime).count() << "ms" << std::endl;
			OutputDebugStringA(str.str().c_str());
			wasReady = true;
		}else{
			// devcon->Flush();
			// draw the vertex buffer to the back buffer
    
			auto tex = GetTexture(nextToRender);
			if(tex!=nullptr){
				ID3D11ShaderResourceView * shaderResourceView;
				auto hr = dev->CreateShaderResourceView(tex,nullptr,&shaderResourceView);
				if(FAILED(hr)){
					throw std::exception("Coudln't create shader resource view");
				}
				devcon->PSSetShaderResources(0,1,&shaderResourceView);
				devcon->PSSetShader(pPS, 0, 0);
				devcon->IASetVertexBuffers(0, 1, &pVBuffer, &stride, &offset);
				devcon->Draw(6, 0);
				shaderResourceView->Release();
				tex->Release();
			}else{
				OutputDebugStringA((std::string("Got null texture for ") + files[nextToRender] + "\n").c_str());
			}
			player->SendNextFrameToLoad(files[nextToLoad]);
			player->Update();
			nextToLoad+=1;
			nextToLoad %= files.size();
			nextToRender+=1;
			nextToRender %= files.size();
		}
	}else{
		OutputDebugStringA(("Player not ready: " + player->GetStatusMessage() + "\n").c_str());
	}

	//OutputDebugStringA((files[nextToRender] + "\n").c_str());

	UpdateVerticalLine();
    devcon->PSSetShader(pPSNoTex, 0, 0);
    devcon->IASetVertexBuffers(0, 1, &pVBufferVerticalLine, &stride, &offset);
	devcon->Draw(6, 0);

    // switch the back buffer and the front buffer
    swapchain->Present(1, 0);

	/*if(i%240==0){
		player = std::make_shared<DX11Player>("D:\\TestMaterial\\bbb_4ktga_crop1");
		player->SetInternalRate(0);
		ready = false;
	}*/
}


// this is the function that cleans up Direct3D and COM
void CleanD3D(void)
{
    swapchain->SetFullscreenState(FALSE, NULL);    // switch to windowed mode

    // close and release all existing COM objects
    //pLayout->Release();
    pVS->Release();
    pPS->Release();
    pVBuffer->Release();
    swapchain->Release();
    backbuffer->Release();
    dev->Release();
    devcon->Release();
}


// this is the function that creates the shape to render
void InitGraphics()
{

	D3D11_BUFFER_DESC vertexBufferDescription;
	vertexBufferDescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDescription.ByteWidth = 6 * sizeof(Vertex);
	vertexBufferDescription.CPUAccessFlags = 0;
	vertexBufferDescription.MiscFlags = 0;
	vertexBufferDescription.StructureByteStride = 0;
	vertexBufferDescription.Usage = D3D11_USAGE_DEFAULT;
		
	Vertex quad[] = {
		Vertex(DirectX::XMFLOAT4(-1.0f, 1.0f, 0.5f, 1.0f),DirectX::XMFLOAT2(0.0f, 0.0f)),
		Vertex(DirectX::XMFLOAT4(1.0f, 1.0f, 0.5f, 1.0f),DirectX::XMFLOAT2(1.0f, 0.0f)),
		Vertex(DirectX::XMFLOAT4(-1.0f, -1.0f, 0.5f, 1.0f),DirectX::XMFLOAT2(0.0f, 1.0f)),

		Vertex(DirectX::XMFLOAT4(-1.0f, -1.0f, 0.5f, 1.0f),DirectX::XMFLOAT2(0.0f, 1.0f)),
		Vertex(DirectX::XMFLOAT4(1.0f, 1.0f, 0.5f, 1.0f),DirectX::XMFLOAT2(1.0f, 0.0f)),
		Vertex(DirectX::XMFLOAT4(1.0f, -1.0f, 0.5f, 1.0f),DirectX::XMFLOAT2(1.0f, 1.0f)),
	};

	D3D11_SUBRESOURCE_DATA bufferData;
	bufferData.pSysMem = quad;
	bufferData.SysMemPitch = 0;
	bufferData.SysMemSlicePitch = 0;
		
	HRESULT hr = dev->CreateBuffer(&vertexBufferDescription,&bufferData,&pVBuffer);

	
		
	Vertex verticalLine[] = {
		Vertex(DirectX::XMFLOAT4(-1.0f, 1.0f, 0.5f, 1.0f),DirectX::XMFLOAT2(0.0f, 0.0f)),
		Vertex(DirectX::XMFLOAT4(-0.9f, 1.0f, 0.5f, 1.0f),DirectX::XMFLOAT2(1.0f, 0.0f)),
		Vertex(DirectX::XMFLOAT4(-1.0f, -1.0f, 0.5f, 1.0f),DirectX::XMFLOAT2(0.0f, 1.0f)),

		Vertex(DirectX::XMFLOAT4(-1.0f, -1.0f, 0.5f, 1.0f),DirectX::XMFLOAT2(0.0f, 1.0f)),
		Vertex(DirectX::XMFLOAT4(-0.9f, 1.0f, 0.5f, 1.0f),DirectX::XMFLOAT2(1.0f, 0.0f)),
		Vertex(DirectX::XMFLOAT4(-0.9f, -1.0f, 0.5f, 1.0f),DirectX::XMFLOAT2(1.0f, 1.0f)),
	};
	bufferData.pSysMem = verticalLine;
	bufferData.SysMemPitch = 0;
	bufferData.SysMemSlicePitch = 0;
	vertexBufferDescription.Usage               = D3D11_USAGE_DYNAMIC;
	vertexBufferDescription.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
	hr = dev->CreateBuffer(&vertexBufferDescription,&bufferData,&pVBufferVerticalLine);

    // select which vertex buffer to display
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    devcon->IASetVertexBuffers(0, 1, &pVBuffer, &stride, &offset);

    // select which primtive type we are using
    devcon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}


// this function loads and prepares the shaders
void InitPipeline()
{
    // load and compile the two shaders
	HRESULT hr = dev->CreateVertexShader(VShader,sizeof(VShader),nullptr,&pVS);
	if(FAILED(hr)){
		throw std::exception("Coudln't create vertex shader\n");
	}
	OutputDebugString( "Created vertex shader\n" );

	hr = dev->CreatePixelShader(PShader,sizeof(PShader),nullptr,&pPS);
	if(FAILED(hr)){
		throw std::exception("Coudln't create pixel shader\n");
	}

	hr = dev->CreatePixelShader(PShaderNoTex,sizeof(PShaderNoTex),nullptr,&pPSNoTex);
	if(FAILED(hr)){
		throw std::exception("Coudln't create pixel shader\n");
	}
	
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{"POSITION",0,DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}, 
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(DirectX::XMFLOAT4), D3D11_INPUT_PER_VERTEX_DATA, 0},
	};
	
	ID3D11InputLayout * vertexLayout;
	hr = dev->CreateInputLayout(layout, 2, VShader, sizeof(VShader), &vertexLayout);
	if(FAILED(hr)){
		throw std::exception("Coudln't create input layout\n");
		//std::cout << "Coudln't create input layout" << std::endl;
		
	}
	OutputDebugString( "Created vertex layout\n" );

	devcon->IASetInputLayout(vertexLayout);
	OutputDebugString( "Set vertex layout\n" );
	vertexLayout->Release();
	

    // set the shader objects
    devcon->VSSetShader(pVS, 0, 0);
    devcon->PSSetShader(pPS, 0, 0);
	
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
	hr = dev->CreateSamplerState(&samplerDesc,&samplerState);
	if(FAILED(hr)){
		throw std::exception("Coudln't create sampler\n");
	}
	OutputDebugString( "Created sampler state\n" );

	devcon->PSSetSamplers(0,1,&samplerState);
	OutputDebugString( "Set sampler state\n" );
	samplerState->Release();

	tinydir_dir dir;
	tinydir_open(&dir, "D:\\TestMaterialArturo\\4097x2160bc1");

	while (dir.has_next)
	{
		tinydir_file file;
		tinydir_readfile(&dir, &file);

		if (!file.is_dir)
		{
			files.push_back(file.path);
		}

		tinydir_next(&dir);
	}

	tinydir_close(&dir);

	player = std::make_shared<DX11Player>(files[0], bufferSize);
}
