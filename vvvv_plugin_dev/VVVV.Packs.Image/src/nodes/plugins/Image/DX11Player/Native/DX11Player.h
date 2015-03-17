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
#include "HighResClock.h"
#include "ImageSequence.h"
#include <memory>

class DX11Player {
public:
	DX11Player(const std::string & directory);
	~DX11Player();
	void OnRender();
	HANDLE GetSharedHandle();
	int GetUploadBufferSize() const;
	int GetWaitBufferSize() const;
	int GetRenderBufferSize() const;
	int GetDroppedFrames() const;
	size_t GetCurrentLoadFrame() const;
	size_t GetCurrentRenderFrame() const;
	void SetFPS(int fps);
	std::string GetDirectory() const;
	int GetAvgLoadDurationMs() const;
	void SendNextFrameToLoad(int nextFrame);
	void SetInternalRate(int enabled);
	bool IsReady() const;
private:
	std::shared_ptr<ImageSequence> m_Sequence;
	ID3D11Device * m_Device;
	ID3D11DeviceContext * m_Context;
	std::vector<ID3D11Texture2D *> m_CopyTextureIn;
	std::vector<ID3D11ShaderResourceView *> m_ShaderResourceViews;
    ID3D11Texture2D* m_BackBuffer;
	ID3D11RenderTargetView*  m_RenderTargetView;
    ID3D11Texture2D* m_TextureBack;
	std::thread m_UploaderThread;
	std::thread m_WaiterThread;
	std::thread m_RateThread;
	bool m_UploaderThreadRunning;
	bool m_WaiterThreadRunning;
	bool m_RateThreadRunning;
	D3D11_BOX m_CopyBox;

	class Frame{
		struct Data{
			static int nextidx;
			int idx;
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
			ID3D11Device * device;
			ID3D11DeviceContext * context;
			bool mapped;

			Data(ID3D11Device * device,ID3D11DeviceContext * context, int w, int h, DXGI_FORMAT format);

			~Data();
			

			HRESULT CreateStagingTexture(int Width, int Height, DXGI_FORMAT Format, ID3D11Texture2D ** texture);
		};
		std::shared_ptr<Data> data;

	public:
		Frame();

		Frame(ID3D11Device * device,ID3D11DeviceContext * context, int w, int h, DXGI_FORMAT format);

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

		void Reset();

		operator bool() const{
			return data!=nullptr;
		}
	};

	Channel<Frame> m_ReadyToUpload;
	Channel<Frame> m_ReadyToWait;
	Channel<Frame> m_ReadyToRate;
	Channel<Frame> m_ReadyToRender;
	Channel<size_t> m_NextFrameChannel;
	bool m_ExternalRate;
	bool m_InternalRate;
	std::map<ID3D11Texture2D*,HANDLE> m_SharedHandles;
	int m_CurrentOutFront;
	size_t m_NextRenderFrame;
	int m_DroppedFrames;
	int m_Fps;
	size_t m_CurrentFrame;
	HighResClock::duration m_AvgDecodeDuration;
	HighResClock::duration m_AvgPipelineLatency;

	HRESULT CreateStagingTexture(int Width, int Height, DXGI_FORMAT Format, ID3D11Texture2D ** texture);
};

extern "C"{
	typedef void * DX11HANDLE;
	NATIVE_API DX11HANDLE DX11Player_Create(const char * directory);
	NATIVE_API void DX11Player_Destroy(DX11HANDLE player);
	NATIVE_API void DX11Player_OnRender(DX11HANDLE player);
	NATIVE_API HANDLE DX11Player_GetSharedHandle(DX11HANDLE player);
	
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
}