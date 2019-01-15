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
#include <memory>
#include <deque>
#include <mutex>
#include "HighResClock.h"
#include "Channel.h"

class Context;
class Frame;

class DX11Player {
public:
	enum Status{
		Init = 0,
		Ready,
		FirstFrame,
		Error = -1
	};

	DX11Player(const std::string & fileForFormat, size_t ring_buffer_size);
	~DX11Player();
	void Update();
  void Cleanup();
	HANDLE GetSharedHandle(const std::string & nextFrame);
	int GetUploadBufferSize() const;
	int GetWaitBufferSize() const;
	int GetRenderBufferSize() const;
	int GetPresentBufferSize() const;
	int GetDroppedFrames() const;
	int GetAvgLoadDurationMs() const;
	bool IsReady() const;
	bool GotFirstFrame() const;
	Status GetStatus();
	std::string GetStatusMessage() const;
	std::string GetCurrentLoadFrame() const;
	std::string GetCurrentRenderFrame() const;
	void SendNextFrameToLoad(const std::string & nextFrame);
	void SetSystemFrames(std::vector<std::string> & frames);
	void SetAlwaysShowLastFrame(bool alwaysShowLastFrame);
private:
	void ChangeStatus(Status code, const std::string & status);
	std::shared_ptr<Context> m_Context;
	std::thread m_UploaderThread;
	std::thread m_WaiterThread;
	bool m_UploaderThreadRunning;
	bool m_WaiterThreadRunning;
	Channel<std::shared_ptr<Frame>> m_ReadyToUpload;
	Channel<std::pair<int,std::shared_ptr<Frame>>> m_ReadyToWait;
	Channel<std::shared_ptr<Frame>> m_ReadyToRender;
	Channel<std::string> m_NextFrameChannel;
	std::vector<std::string> m_SystemFrames;
	std::map<std::string,std::shared_ptr<Frame>> m_WaitingToPresent;
	std::string m_NextRenderFrame;
	int m_DroppedFrames;
	size_t m_RingBufferSize;
	std::string m_CurrentFrame;
	HighResClock::duration m_AvgDecodeDuration;
	HighResClock::duration m_AvgPipelineLatency;
	HighResClock::time_point m_LastUpdateTime;
	bool m_GotFirstFrame;
	std::string m_StatusDesc;
	Status m_StatusCode;
	std::mutex m_SystemFramesMutex;
	bool m_AlwaysShowLastFrame;
};
