#pragma once
#include <vector>
#include "HighResClock.h"
#include <memory>
#include <d3d11.h>
class Context;

class Frame{
	HighResClock::time_point presentationTime;
	HighResClock::time_point loadTime;
	HighResClock::duration decodeDuration;
	size_t nextToLoad;
	int fps;
	OVERLAPPED overlap;
	HANDLE waitEvent;
	HANDLE file;
	HANDLE renderTextureSharedHandle;
	D3D11_MAPPED_SUBRESOURCE mappedBuffer;
	ID3D11Texture2D* uploadBuffer;
	ID3D11Texture2D* renderTexture;
	Context * context;
	bool mapped;
	bool readyToPresent;
	HRESULT Map();
	void Unmap();
	friend void ReleaseFrame(Frame * frame);

public:
	Frame(Context * context);
	~Frame();
	void Reset();
	bool IsReadyToPresent();
	void Render();
	void SetNextToLoad(size_t next);
	size_t NextToLoad() const;
	HighResClock::time_point LoadTime() const;
	HighResClock::time_point PresentationTime() const;
	ID3D11Texture2D* UploadBuffer();
	ID3D11Texture2D* RenderTexture();
	HANDLE RenderTextureSharedHandle();
	bool ReadFile(const std::string & path, size_t offset, DWORD numbytesdata, HighResClock::time_point now, HighResClock::time_point presentationTime, int currentFps);
	bool Wait(DWORD millis);
	void Cancel();
	HighResClock::duration DecodeDuration() const;
	int Fps() const;
	size_t GetMappedRowPitch() const;
};

