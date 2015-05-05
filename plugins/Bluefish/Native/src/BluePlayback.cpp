// BluePlayback.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "BlueVelvet4.h"

#include "BluePlayback.h"
#include "PlaybackDevice.h"
#include "BlueSDIOut.h"
#include "BlueFishRendererDX11.h"
#include "BlueFishSwapChain.h"

#ifdef _MANAGED
#pragma managed(push, off)
#endif

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
    return TRUE;
}

#ifdef _MANAGED
#pragma managed(pop)
#endif


extern "C"
{
	BLUEPLAYBACK_API BLUEPLAYBACK_HANDLE BluePlaybackCreate(int num_bluefish_buffers)
	{
		int DevCount = 0;
		CPlaybackDevice * pBluePlayback = NULL;
		pBluePlayback = new CPlaybackDevice(num_bluefish_buffers);

		if(pBluePlayback)
			DevCount = pBluePlayback->CheckCardPresent();
		else
			return NULL;

		if(DevCount)
			return pBluePlayback;
		else
		{
			delete pBluePlayback;
			return NULL;
		}

		return pBluePlayback;
	}

	BLUEPLAYBACK_API int BluePlaybackConfig(BLUEPLAYBACK_HANDLE pBPHandle, INT32 inDevNo,
																			INT32 inChannel,
																			INT32 inVidFmt,
																			INT32 inMemFmt,
																			INT32 inUpdFmt,
																			INT32 inVideoDestination,
																			INT32 inAudioDestination,
																			INT32 inAudioChannelMask)
	{
		CPlaybackDevice * pBluePlayback = (CPlaybackDevice *)pBPHandle;
		if(!pBluePlayback)
			return -1;

		return pBluePlayback->Config(inDevNo, inChannel, inVidFmt, inMemFmt, inUpdFmt, inVideoDestination, inAudioDestination, inAudioChannelMask);
	}

	BLUEPLAYBACK_API int BluePlaybackStart(BLUEPLAYBACK_HANDLE pBPHandle)
	{
		CPlaybackDevice * pBluePlayback = (CPlaybackDevice *)pBPHandle;
		if(!pBluePlayback)
			return -1;

		return pBluePlayback->Start();
	}

	BLUEPLAYBACK_API int BluePlaybackStop(BLUEPLAYBACK_HANDLE pBPHandle)
	{
		CPlaybackDevice * pBluePlayback = (CPlaybackDevice *)pBPHandle;
		if(!pBluePlayback)
			return -1;

		return pBluePlayback->Stop();
	}

	BLUEPLAYBACK_API int BluePlaybackWaitSync(BLUEPLAYBACK_HANDLE pBPHandle)
	{
		CPlaybackDevice * pBluePlayback = (CPlaybackDevice *)pBPHandle;
		if(!pBluePlayback)
			return -1;

		pBluePlayback->WaitSync();
		return 0;
	}

	BLUEPLAYBACK_API int BluePlaybackWaitGlobalSync(int deviceNum)
	{
		BlueFishSwapChain::GetSwapChain(deviceNum)->WaitSync();
		return 0;
	}

	BLUEPLAYBACK_API int BluePlaybackUpload(BLUEPLAYBACK_HANDLE pBPHandle, BYTE* pBuffer)
	{
		CPlaybackDevice * pBluePlayback = (CPlaybackDevice *)pBPHandle;
		if(!pBluePlayback || !pBuffer)
			return -1;

		pBluePlayback->Upload(pBuffer);
		return 0;
	}

	BLUEPLAYBACK_API int BluePlaybackRenderNext(BLUEPLAYBACK_HANDLE pBPHandle){
		CPlaybackDevice * pBluePlayback = (CPlaybackDevice *)pBPHandle;
		if(!pBluePlayback)
			return -1;

		pBluePlayback->RenderNext();
		return 0;
	}

	BLUEPLAYBACK_API void BluePlaybackDestroy(BLUEPLAYBACK_HANDLE pBPHandle)
	{
		CPlaybackDevice * pBluePlayback = (CPlaybackDevice *)pBPHandle;

		if(pBluePlayback)
			delete pBluePlayback;
	}

	BLUEPLAYBACK_API int BluePlaybackSetCardProperty(BLUEPLAYBACK_HANDLE pBPHandle, int nProp, unsigned int nValue)
	{
		CPlaybackDevice * pBluePlayback = (CPlaybackDevice *)pBPHandle;

		if (!pBluePlayback)
			return -1;

		return pBluePlayback->SetCardProperty(nProp, nValue);
	}

	BLUEPLAYBACK_API int BluePlaybackSetCardPropertyInt64(BLUEPLAYBACK_HANDLE pBPHandle, int nProp, __int64 nValue)
	{
		CPlaybackDevice * pBluePlayback = (CPlaybackDevice *)pBPHandle;

		if (!pBluePlayback)
			return -1;

		return pBluePlayback->SetCardProperty(nProp, nValue);
	}

	BLUEPLAYBACK_API int BluePlaybackQueryULongCardProperty(BLUEPLAYBACK_HANDLE pBPHandle, int nProp, ULONG & outValue)
	{
		CPlaybackDevice * pBluePlayback = (CPlaybackDevice *)pBPHandle;

		if (!pBluePlayback)
			return -1;

		VARIANT varVal;
		int error_code;
		error_code = pBluePlayback->QueryCardProperty(nProp, &varVal);

		printf("query card property error: %d\n", error_code);
		printf("vartype: %d\n", varVal.vt);
		printf("var value: %d\n", varVal.ulVal);
		
		outValue = varVal.ulVal;

		return error_code;
	}

	BLUEPLAYBACK_API int BluePlaybackQueryUIntCardProperty(BLUEPLAYBACK_HANDLE pBPHandle, int nProp, UINT32 & outValue)
	{
		CPlaybackDevice * pBluePlayback = (CPlaybackDevice *)pBPHandle;

		if (!pBluePlayback)
			return -1;

		VARIANT varVal;
		int error_code;
		error_code = pBluePlayback->QueryCardProperty(nProp, &varVal);

		printf("query card property error: %d\n", error_code);
		printf("vartype: %d\n", varVal.vt);
		printf("var value: %d\n", varVal.ulVal);
		
		outValue = varVal.uintVal;

		return error_code;
	}

	BLUEPLAYBACK_API ULONG BluePlaybackGetMemorySize(BLUEPLAYBACK_HANDLE pBPHandle)
	{
		CPlaybackDevice * pBluePlayback = (CPlaybackDevice *)pBPHandle;

		if (!pBluePlayback)
			return -1;

		return pBluePlayback->GetMemorySize();
	}


	BLUEPLAYBACK_API const char* BluePlaybackGetSerialNumber(BLUEPLAYBACK_HANDLE pBPHandle)
	{
		CPlaybackDevice * pBluePlayback = (CPlaybackDevice *)pBPHandle;

		if (!pBluePlayback)
			return NULL;


		printf("alloc mem for serial number\n");
		int serialNumberSize = 20;
		char *serialNumber = (char*)malloc(serialNumberSize);		
		ZeroMemory(serialNumber, serialNumberSize);

		printf("trying to get serial number...\n");
		int res = pBluePlayback->GetCardSerialNumber(serialNumber, serialNumberSize);
		
		printf("serial number: %s\n", serialNumber);

		return serialNumber;
	}
	

	BLUEPLAYBACK_API const char* BluePlaybackBlueVelvetVersion()
	{
		return BlueVelvetVersion();
	}


	BLUEPLAYBACK_API int BluePlaybackGetDeviceCount(BLUEPLAYBACK_HANDLE pBPHandle)
	{
		CPlaybackDevice * pBluePlayback = (CPlaybackDevice *)pBPHandle;

		if (!pBluePlayback)
			return 0;


		int DevCount = 0;
		DevCount = pBluePlayback->CheckCardPresent();

		return DevCount;
	}

	BLUEPLAYBACK_API unsigned long BluePlaybackGetPixelsPerLine(BLUEPLAYBACK_HANDLE pBPHandle)
	{
		CPlaybackDevice * pBluePlayback = (CPlaybackDevice *)pBPHandle;

		if (!pBluePlayback)
			return 0;

		return pBluePlayback->GetPixelsPerLine();

	}

	BLUEPLAYBACK_API unsigned long BluePlaybackGetVideoLines(BLUEPLAYBACK_HANDLE pBPHandle)
	{
		CPlaybackDevice * pBluePlayback = (CPlaybackDevice *)pBPHandle;

		if (!pBluePlayback)
			return 0;

		return pBluePlayback->GetVideoLines();

	}

	BLUEPLAYBACK_API unsigned long BluePlaybackGetBytesPerLine(BLUEPLAYBACK_HANDLE pBPHandle)
	{
		
		CPlaybackDevice * pBluePlayback = (CPlaybackDevice *)pBPHandle;

		if (!pBluePlayback)
			return 0;

		return pBluePlayback->GetBytesPerLine();
	}

	BLUEPLAYBACK_API unsigned long BluePlaybackGetBytesPerFrame(BLUEPLAYBACK_HANDLE pBPHandle)
	{
		
		CPlaybackDevice * pBluePlayback = (CPlaybackDevice *)pBPHandle;

		if (!pBluePlayback)
			return 0;

		return pBluePlayback->GetBytesPerFrame();
	}


	BLUEPLAYBACK_API BLUEPLAYBACK_HANDLE BlueSDIOutCreate(int device, int sdi_out)
	{
		return new CPlaybackSDIOut(device,sdi_out);
	}

	BLUEPLAYBACK_API void BlueSDIOutRoute(BLUEPLAYBACK_HANDLE pBPHandle, int mem_channel)
	{
		CPlaybackSDIOut * pBlueSDIOut = (CPlaybackSDIOut *)pBPHandle;

		if (!pBlueSDIOut)
			return;

		pBlueSDIOut->Route(mem_channel);
	}

	BLUEPLAYBACK_API void BlueSDIOutSetTransport(BLUEPLAYBACK_HANDLE pBPHandle, unsigned int transport)
	{
		CPlaybackSDIOut * pBlueSDIOut = (CPlaybackSDIOut *)pBPHandle;

		if (!pBlueSDIOut)
			return;

		pBlueSDIOut->SetTransport((EHdSdiTransport)transport);
	}

	
	BLUEPLAYBACK_API BLUERENDERER_HANDLE BlueRendererCreate(HANDLE tex_handle, int outFormat, int num_render_target_buffers, int num_read_back_buffers, int num_bluefish_buffers)
	{
		try{
			return new BlueFishRendererDX11(tex_handle,(EMemoryFormat)outFormat, num_render_target_buffers, num_read_back_buffers, num_bluefish_buffers);
		}catch (std::exception & e){
			WCHAR    str[1024];
			MultiByteToWideChar( 0,0, e.what(), (int)strlen(e.what()), str, (int)strlen(e.what())+1);
			OutputDebugString( str );
			return 0;
		}
	}

	
	BLUEPLAYBACK_API void BlueRendererDestroy(BLUERENDERER_HANDLE renderer)
	{
		BlueFishRendererDX11 * pBlueRenderer = static_cast<BlueFishRendererDX11*>(renderer);

		if(pBlueRenderer)
			delete pBlueRenderer;
	}

	
	BLUEPLAYBACK_API void BlueRendererStop(BLUERENDERER_HANDLE renderer)
	{
		if(!renderer)
			return;

		static_cast<BlueFishRendererDX11*>(renderer)->Stop();
	}

	
	BLUEPLAYBACK_API void BlueRendererSetSharedHandle(BLUERENDERER_HANDLE renderer, HANDLE tex_handle)
	{
		if(!renderer)
			return;

		static_cast<BlueFishRendererDX11*>(renderer)->SetSharedHandle(tex_handle);
	}

	BLUEPLAYBACK_API BLUEPLAYBACK_HANDLE BlueRendererGetPlaybackDevice(BLUERENDERER_HANDLE renderer)
	{
		try{
			return static_cast<BlueFishRendererDX11*>(renderer)->GetDevice();
		}catch (std::exception & e){
			WCHAR    str[1024];
			MultiByteToWideChar( 0,0, e.what(), (int)strlen(e.what()), str, (int)strlen(e.what())+1);
			OutputDebugString( str );
			return 0;
		}
	}

	BLUEPLAYBACK_API DX11_HANDLE BlueRendererGetDX11Device(BLUERENDERER_HANDLE renderer)
	{
		try{
			return static_cast<BlueFishRendererDX11*>(renderer)->GetDX11Device();
		}catch (std::exception & e){
			WCHAR    str[1024];
			MultiByteToWideChar( 0,0, e.what(), (int)strlen(e.what()), str, (int)strlen(e.what())+1);
			OutputDebugString( str );
			return 0;
		}
	}

	BLUEPLAYBACK_API void BlueRendererOnPresent(BLUERENDERER_HANDLE renderer)
	{
		try{
			static_cast<BlueFishRendererDX11*>(renderer)->OnPresent();
		}catch (std::exception & e){
			WCHAR    str[1024];
			MultiByteToWideChar( 0,0, e.what(), (int)strlen(e.what()), str, (int)strlen(e.what())+1);
			OutputDebugString( str );
		}
	}

	BLUEPLAYBACK_API double BlueRendererGetAvgDuration(BLUERENDERER_HANDLE renderer)
	{
		return static_cast<BlueFishRendererDX11*>(renderer)->GetAvgDurationMS();
	}

	BLUEPLAYBACK_API double BlueRendererGetMaxDuration(BLUERENDERER_HANDLE renderer)
	{
		return static_cast<BlueFishRendererDX11*>(renderer)->GetMaxDurationMS();
	}

}	//end of extern "C"
