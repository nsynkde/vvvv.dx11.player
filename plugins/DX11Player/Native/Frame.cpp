#include "stdafx.h"
#include "Frame.h"
#include "Pool.h"
#include "Windows.h"
#include <sstream>

Frame::Frame(Context * context)
:decodeDuration(0)
,waitEvent(CreateEventW(0,false,false,0))
,file(nullptr)
,uploadBuffer(nullptr)
,context(context)
,mapped(false)
,readyToPresent(false){
	HRESULT hr;
	if (context->GetCopyType() == Context::DiskToGPU) {
		hr = context->CreateStagingTexture(&uploadBuffer);
		if (FAILED(hr)) {
			throw std::exception((std::string("Coudln't create staging texture")).c_str());
		}
		ZeroMemory(&mappedBuffer, sizeof(D3D11_MAPPED_SUBRESOURCE));

		if (Map() != 0) {
			throw std::exception("Coudln't map frame\n");
		}
	} else {
		ramUploadBuffer.resize(context->GetFormat().bytes_data + context->GetFormat().data_offset);
	}
	hr = context->CreateRenderTexture(&renderTexture);
	if(FAILED(hr)){
		throw std::exception((std::string("Coudln't create render texture")).c_str());
	}
	// if using our own device, create a shared handle for each texture
	//OutputDebugString( L"getting shared texture handle\n" );
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
}

Frame::~Frame(){
	if (context->GetCopyType() == Context::DiskToGPU) {
		if (mapped) context->GetDX11Context()->Unmap(uploadBuffer, 0);
		uploadBuffer->Release();
	}
	renderTexture->Release();
	if(file!=nullptr){
		WaitForSingleObject(waitEvent,INFINITE);
		CloseHandle(file);
	}
	CloseHandle(waitEvent);
}

void Frame::Cancel(){
	if (file != nullptr){
		auto result = CancelIoEx(file, &overlap);
		if (result == TRUE || GetLastError() != ERROR_NOT_FOUND)
		{
			// Wait for the I/O subsystem to acknowledge our cancellation.
			// Depending on the timing of the calls, the I/O might complete with a
			// cancellation status, or it might complete normally (if the ReadFile was
			// in the process of completing at the time CancelIoEx was called, or if
			// the device does not support cancellation).
			// This call specifies TRUE for the bWait parameter, which will block
			// until the I/O either completes or is canceled, thus resuming execution, 
			// provided the underlying device driver and associated hardware are functioning 
			// properly. If there is a problem with the driver it is better to stop 
			// responding here than to try to continue while masking the problem.

			DWORD bytesRead;
			result = GetOverlappedResult(file, &overlap, &bytesRead, TRUE);

			// ToDo: check result and log errors. 
		}
		CloseHandle(file);
		file = nullptr;
	}
	readyToPresent = false;
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
	if (context->GetCopyType() == Context::DiskToGPU) {
		Unmap();

		context->CopyFrameToOutTexture(this);

		Map();
		readyToPresent = true;
	}
}

size_t Frame::GetMappedRowPitch() const{
	return mappedBuffer.RowPitch;
}

const std::string &  Frame::SourcePath() const{
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
	nextToLoad = "";
	readyToPresent = false;
	if(file != nullptr){
		Cancel();
	}

	uint8_t * ptr;
	if (context->GetCopyType() == Context::DiskToGPU) {
		ptr = (uint8_t*)mappedBuffer.pData;
	} else {
		ramUploadBuffer.assign(ramUploadBuffer.size(), 0);
		ptr = ramUploadBuffer.data();
	}
	if(ptr){
		ptr += offset;
		file = CreateFileA(path.c_str(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED,NULL);
		if (file != INVALID_HANDLE_VALUE) {
			ZeroMemory(&overlap,sizeof(OVERLAPPED));
			overlap.hEvent = waitEvent;
			DWORD bytesRead=0;
			::ReadFile(file, ptr, numbytesdata, &bytesRead, &overlap);
			loadTime = now;
			nextToLoad = path;
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
	auto success = ret != WAIT_TIMEOUT && ret != WAIT_FAILED;
	/*if (success && context->GetCopyType() == Context::DiskToRam) {
		OutputDebugStringA(("Copying " + std::to_string(context->GetFormat().row_pitch) + " x " + std::to_string(context->GetFormat().h) + " to " + std::to_string(mappedBuffer.RowPitch) + "@ " + std::to_string((uint64_t)mappedBuffer.pData) + "\n").c_str());
		auto src = GetRAMBuffer();
		auto dst = (uint8_t*)mappedBuffer.pData;
		for (size_t i = 0; i < context->GetFormat().h; i++) {
			memcpy(dst, src, context->GetFormat().row_pitch);
			src += context->GetFormat().row_pitch;
			dst += mappedBuffer.RowPitch;
		}
	}*/

	if (success && context->GetCopyType() == Context::DiskToRam) {
		context->CopyFrameToOutTexture(this);
		readyToPresent = true;
	}
	return success;
}

HighResClock::duration Frame::DecodeDuration() const{
	return decodeDuration;
}

uint8_t * Frame::GetRAMBuffer() {
	return ramUploadBuffer.data() + context->GetFormat().data_offset;
}