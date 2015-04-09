// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the NATIVE_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// NATIVE_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef NATIVE_EXPORTS
#define NATIVE_API __declspec(dllexport)
#else
#define NATIVE_API __declspec(dllimport)
#endif

#include <string>
#include <vector>
#include <thread>
#include <d3d11.h>
#include <map>
#include "Channel.h"
#include "ImageSequence.h"
#include <memory>
#include <deque>
#include "Frame.h"
#include "Pool.h"

class DX11Player {
public:
	enum Status{
		Init = 0,
		Ready,
		FirstFrame,
		Error = -1
	};

	DX11Player(const std::string & directory, const std::string & wildcard, size_t ring_buffer_size);
	~DX11Player();
	void Update();
	HANDLE GetSharedHandle(int nextFrame);
	int GetUploadBufferSize() const;
	int GetWaitBufferSize() const;
	int GetRenderBufferSize() const;
	int GetDroppedFrames() const;
	size_t GetCurrentLoadFrame() const;
	size_t GetCurrentRenderFrame() const;
	void SetFPS(int fps);
	std::string GetDirectory() const;
	int GetAvgLoadDurationMs() const;
	void SetInternalRate(int enabled);
	bool IsReady() const;
	bool GotFirstFrame() const;
	Status GetStatus();
	std::string GetStatusMessage();
	void SendNextFrameToLoad(int nextFrame);
private:
	void ChangeStatus(Status code, const std::string & status);
	std::shared_ptr<ImageSequence> m_Sequence;
	std::shared_ptr<Context> m_Context;
	std::thread m_UploaderThread;
	std::thread m_WaiterThread;
	std::thread m_RateThread;
	bool m_UploaderThreadRunning;
	bool m_WaiterThreadRunning;
	bool m_RateThreadRunning;
	Channel<std::shared_ptr<Frame>> m_ReadyToUpload;
	Channel<std::shared_ptr<Frame>> m_ReadyToWait;
	Channel<std::shared_ptr<Frame>> m_ReadyToRate;
	Channel<std::shared_ptr<Frame>> m_ReadyToRender;
	Channel<size_t> m_NextFrameChannel;
	std::vector<size_t> m_SystemFrames;
	std::map<size_t,std::shared_ptr<Frame>> m_WaitingToPresent;
	bool m_ExternalRate;
	bool m_InternalRate;
	size_t m_NextRenderFrame;
	int m_DroppedFrames;
	int m_Fps;
	size_t m_RingBufferSize;
	size_t m_CurrentFrame;
	HighResClock::duration m_AvgDecodeDuration;
	HighResClock::duration m_AvgPipelineLatency;
	HighResClock::time_point m_LastUpdateTime;
	bool m_GotFirstFrame;
	std::string m_StatusDesc;
	Status m_StatusCode;
	std::mutex m_SystemFramesMutex;
};

extern "C"{
	typedef void * DX11HANDLE;
	NATIVE_API DX11HANDLE DX11Player_Create(const char * directory, const char * wildcard, int ringBufferSize);
	NATIVE_API void DX11Player_Destroy(DX11HANDLE player);
	NATIVE_API void DX11Player_Update(DX11HANDLE player);
	NATIVE_API HANDLE DX11Player_GetSharedHandle(DX11HANDLE player,int nextFrame);
	
	NATIVE_API const char * DX11Player_GetDirectory(DX11HANDLE player);
	NATIVE_API int DX11Player_DirectoryHasChanged(DX11HANDLE player, const char * dir);
	NATIVE_API int DX11Player_GetUploadBufferSize(DX11HANDLE player);
	NATIVE_API int DX11Player_GetWaitBufferSize(DX11HANDLE player);
	NATIVE_API int DX11Player_GetRenderBufferSize(DX11HANDLE player);
	NATIVE_API int DX11Player_GetDroppedFrames(DX11HANDLE player);
	NATIVE_API int DX11Player_GetCurrentLoadFrame(DX11HANDLE player);
	NATIVE_API int DX11Player_GetCurrentRenderFrame(DX11HANDLE player);
	NATIVE_API int DX11Player_GetAvgLoadDurationMs(DX11HANDLE player);
	NATIVE_API void DX11Player_SetFPS(DX11HANDLE player, int fps);
	NATIVE_API void DX11Player_SendNextFrameToLoad(DX11HANDLE player, int nextFrame);
	NATIVE_API void DX11Player_SetInternalRate(DX11HANDLE player, int enabled);
	NATIVE_API bool DX11Player_IsReady(DX11HANDLE player);
	NATIVE_API bool DX11Player_GotFirstFrame(DX11HANDLE player);
	NATIVE_API int DX11Player_GetStatus(DX11HANDLE player);
	NATIVE_API const char * DX11Player_GetStatusMessage(DX11HANDLE player);
}