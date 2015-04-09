#include "stdafx.h"
#include "Frame.h"
#include "Pool.h"
#include "Windows.h"

Frame::Frame(Context * context)
:decodeDuration(0)
,waitEvent(CreateEventW(0,false,false,0))
,file(nullptr)
,uploadBuffer(nullptr)
,context(context)
,mapped(false)
,readyToPresent(false){
	HRESULT hr = context->CreateStagingTexture(&uploadBuffer);
	if(FAILED(hr)){
		throw std::exception((std::string("Coudln't create staging texture")).c_str());
	}
	hr = context->CreateRenderTexture(&renderTexture);
	if(FAILED(hr)){
		throw std::exception((std::string("Coudln't create render texture")).c_str());
	}
	// if using our own device, create a shared handle for each texture
	OutputDebugString( L"getting shared texture handle\n" );
	IDXGIResource* pTempResource(NULL);
	hr = renderTexture->QueryInterface( __uuidof(IDXGIResource), (void**)&pTempResource );
	if(FAILED(hr)){
		throw std::exception("Coudln't query interface\n");
	}
	hr = pTempResource->GetSharedHandle(&renderTextureSharedHandle);
	if(FAILED(hr)){
		throw std::exception("Coudln't get shared handle\n");
	}
	pTempResource->Release();
	ZeroMemory(&overlap,sizeof(OVERLAPPED));
	ZeroMemory(&mappedBuffer,sizeof(D3D11_MAPPED_SUBRESOURCE));
	
	if(Map()!=0){
		throw std::exception("Coudln't map frame\n");
	}
}

Frame::~Frame(){
	if(mapped) context->GetDX11Context()->Unmap(uploadBuffer, 0);
	uploadBuffer->Release();
	renderTexture->Release();
	if(file!=nullptr){
		WaitForSingleObject(waitEvent,INFINITE);
		CloseHandle(waitEvent);
		CloseHandle(file);
	}
}

void Frame::Reset(){
	SetNextToLoad("");
	readyToPresent = false;
}

void Frame::Cancel(){
	CancelIoEx(file,&overlap);
	CloseHandle(file);
	Reset();
}

bool Frame::IsReadyToPresent(){
	return readyToPresent;
}

HRESULT Frame::Map(){
	if(mapped) return 0;
	mapped = true;
	return context->GetDX11Context()->Map(uploadBuffer,0,D3D11_MAP_WRITE,0,&mappedBuffer);
}

void Frame::Unmap(){
	if(!mapped) return;
	mapped = false;
	context->GetDX11Context()->Unmap(uploadBuffer, 0);
}

void Frame::Render(){
	Unmap();
	context->CopyFrameToOutTexture(this);
	Map();
	readyToPresent = true;
}

size_t Frame::GetMappedRowPitch() const{
	return mappedBuffer.RowPitch;
}

void Frame::SetNextToLoad(const std::string & next){
	nextToLoad = next;
}

const std::string &  Frame::NextToLoad() const{
	return nextToLoad;
}

HighResClock::time_point Frame::LoadTime() const{
	return loadTime;
}

ID3D11Texture2D* Frame::UploadBuffer(){
	return uploadBuffer;
}

ID3D11Texture2D* Frame::RenderTexture(){
	return renderTexture;
}

HANDLE Frame::RenderTextureSharedHandle(){
	return renderTextureSharedHandle;
}

bool Frame::ReadFile(const std::string & path, size_t offset, DWORD numbytesdata, HighResClock::time_point now){
	if(file != nullptr){
		Wait(INFINITE);
	}
	auto ptr = (char*)mappedBuffer.pData;
	if(ptr){
		ptr += offset;
		file = CreateFileA(path.c_str(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED,NULL);
		if (file != INVALID_HANDLE_VALUE) {
			ZeroMemory(&overlap,sizeof(OVERLAPPED));
			overlap.hEvent = waitEvent;
			DWORD bytesRead=0;
			::ReadFile(file,ptr,numbytesdata,&bytesRead,&overlap);
			loadTime = now;
			return true;
		}
	}
	return false;
}

bool Frame::Wait(DWORD millis){
	if(file==nullptr) return false;
	auto ret = WaitForSingleObject(waitEvent,millis);
	decodeDuration = HighResClock::now() - loadTime;
	CloseHandle(file);
	file = nullptr;
	return ret != WAIT_TIMEOUT && ret != WAIT_FAILED;
}

HighResClock::duration Frame::DecodeDuration() const{
	return decodeDuration;
}