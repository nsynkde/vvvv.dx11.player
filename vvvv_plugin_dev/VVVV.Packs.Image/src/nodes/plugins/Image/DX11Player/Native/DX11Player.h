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

class DX11Player {
public:
	DX11Player(ID3D11Device * device, const std::string & directory);
	void OnRender();
	HANDLE GetSharedHandle();
	ID3D11Texture2D * GetTexturePointer();
	ID3D11Texture2D * GetRenderTexturePointer();
	void DoneRender();
	int GetUploadBufferSize();
	int GetWaitBufferSize();
	int GetRenderBufferSize();

private:
	ID3D11Device * m_Device;
	ID3D11DeviceContext * m_Context;
	std::vector<ID3D11Texture2D *> m_CopyTextureIn;
	std::vector<ID3D11Texture2D *> m_CopyTextureOut;
	std::vector<ID3D11ShaderResourceView *> m_ShaderResourceView;
	std::vector<ID3D11UnorderedAccessView *> m_UAVs;
	std::vector<ID3D11Texture2D *> m_OutputTextures;
	//std::vector<ID3D11Buffer*> m_UploadBuffers;
	std::vector<ID3D11Texture2D*> m_UploadBuffers;
	std::vector<OVERLAPPED> m_Overlaps;
	std::vector<HANDLE> m_WaitEvents;
	std::vector<HANDLE> m_FileHandles;
	std::vector<D3D11_MAPPED_SUBRESOURCE> m_MappedBuffers;
	std::vector<std::string> m_ImageFiles;
	std::thread m_UploaderThread;
	std::thread m_WaiterThread;
	bool m_UploaderThreadRunning;
	bool m_WaiterThreadRunning;
	size_t m_CurrentFrame;

	Channel<int> m_ReadyToUpload;
	Channel<int> m_ReadyToWait;
	Channel<int> m_ReadyToRender;
	bool m_NewFrame;
	std::map<ID3D11Texture2D*,HANDLE> m_SharedHandles;
	int m_NextRenderBuffer;
	HighResClock::time_point m_LastFrame;
	double m_FramesRemainder;
	int m_CurrentOutFront;
	int m_CurrentOutBack;
	std::vector<int> m_ConsumedBuffers;

	HRESULT CreateStagingTexture(int Width, int Height, DXGI_FORMAT Format, ID3D11Texture2D ** texture);
	HRESULT CreateStagingBuffer(int Width, int Height, DXGI_FORMAT Format, ID3D11Buffer ** buffer);
	HRESULT CreateCopyBufferIn(int Width, int Height, DXGI_FORMAT Format, ID3D11Buffer ** buffer);
	HRESULT CreateCopyBufferOut(int Width, int Height, DXGI_FORMAT Format, ID3D11Buffer ** buffer);
};

extern "C"{
	typedef void * DX11HANDLE;
	NATIVE_API DX11HANDLE DX11Player_Create(DX11HANDLE device, const char * directory);
	NATIVE_API void DX11Player_Destroy(DX11HANDLE player);
	NATIVE_API void DX11Player_OnRender(DX11HANDLE player);
	NATIVE_API HANDLE DX11Player_GetSharedHandle(DX11HANDLE player);
	NATIVE_API DX11HANDLE DX11Player_GetTexturePointer(DX11HANDLE player);
	NATIVE_API DX11HANDLE DX11Player_GetRenderTexturePointer(DX11HANDLE player);
	NATIVE_API void DX11Player_DoneRender(DX11HANDLE player);

	NATIVE_API int DX11Player_GetUploadBufferSize(DX11HANDLE player);
	NATIVE_API int DX11Player_GetWaitBufferSize(DX11HANDLE player);
	NATIVE_API int DX11Player_GetRenderBufferSize(DX11HANDLE player);

}