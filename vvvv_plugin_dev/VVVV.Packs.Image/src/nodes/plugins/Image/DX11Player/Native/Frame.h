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
	D3D11_MAPPED_SUBRESOURCE mappedBuffer;
	ID3D11Texture2D* uploadBuffer;
	Context * context;
	bool mapped;
	friend void ReleaseFrame(Frame * frame);

public:
	Frame(Context * context);
	~Frame();
	HRESULT Map();
	void Unmap();
	void SetNextToLoad(size_t next);
	size_t NextToLoad() const;
	HighResClock::time_point LoadTime() const;
	HighResClock::time_point PresentationTime() const;
	ID3D11Texture2D* UploadBuffer();
	bool ReadFile(const std::string & path, size_t offset, size_t numbytesdata, HighResClock::time_point now, HighResClock::time_point presentationTime, int currentFps);
	void Wait();
	HighResClock::duration DecodeDuration() const;
	int Fps() const;
};

