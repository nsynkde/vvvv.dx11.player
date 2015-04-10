#pragma once
#include <vector>
#include "HighResClock.h"
#include <memory>
#include <d3d11.h>
class Context;

class Frame{
	HighResClock::time_point loadTime;
	HighResClock::duration decodeDuration;
	std::string nextToLoad;
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
	void Cancel();
	friend void ReleaseFrame(Frame * frame);

public:
	Frame(Context * context);
	~Frame();
	bool IsReadyToPresent();
	void Render();
	const std::string & SourcePath() const;
	HighResClock::time_point LoadTime() const;
	ID3D11Texture2D* UploadBuffer();
	ID3D11Texture2D* RenderTexture();
	HANDLE RenderTextureSharedHandle();
	bool ReadFile(const std::string & path, size_t offset, DWORD numbytesdata, HighResClock::time_point now);
	bool Wait(DWORD millis);
	HighResClock::duration DecodeDuration() const;
	size_t GetMappedRowPitch() const;
};

