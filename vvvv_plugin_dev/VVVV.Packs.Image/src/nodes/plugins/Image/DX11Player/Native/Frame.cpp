#include "stdafx.h"
#include "Frame.h"
#include "Pool.h"
#include "Windows.h"

Frame::Frame(Context * context)
:decodeDuration(0)
,nextToLoad(-1)
,fps(0)
,waitEvent(CreateEventW(0,false,false,0))
,file(nullptr)
,uploadBuffer(nullptr)
,context(context)
,mapped(false){
	HRESULT hr = context->CreateStagingTexture(&uploadBuffer);
	if(FAILED(hr)){
		throw std::exception((std::string("Coudln't create staging texture")).c_str());
	}
	ZeroMemory(&overlap,sizeof(OVERLAPPED));
	ZeroMemory(&mappedBuffer,sizeof(D3D11_MAPPED_SUBRESOURCE));
}

Frame::~Frame(){
	if(mapped) context->GetDX11Context()->Unmap(uploadBuffer, 0);
	uploadBuffer->Release();
	if(file!=nullptr){
		WaitForSingleObject(waitEvent,INFINITE);
		CloseHandle(waitEvent);
		CloseHandle(file);
	}
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

void Frame::SetNextToLoad(size_t next){
	nextToLoad = next;
}

size_t Frame::NextToLoad() const{
	return nextToLoad;
}

HighResClock::time_point Frame::LoadTime() const{
	return loadTime;
}

HighResClock::time_point Frame::PresentationTime() const{
	return presentationTime;
}

ID3D11Texture2D* Frame::UploadBuffer(){
	return uploadBuffer;
}

bool Frame::ReadFile(const std::string & path, size_t offset, size_t numbytesdata, HighResClock::time_point now, HighResClock::time_point presentationTime, int currentFps){
	if(file != nullptr){
		Wait();
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
			fps = currentFps;
			loadTime = now;
			presentationTime = presentationTime;
			return true;
		}
	}
	return false;
}

void Frame::Wait(){
	if(file==nullptr) return;
	WaitForSingleObject(waitEvent,INFINITE);
	decodeDuration = HighResClock::now() - loadTime;
	CloseHandle(file);
	file = nullptr;
}

HighResClock::duration Frame::DecodeDuration() const{
	return decodeDuration;
}

int Frame::Fps() const{
	return fps;
}