// BluePlayback.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "BlueVelvet4.h"
#include "BluePlayback.h"
#include "PlaybackDevice.h"


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
	BLUEPLAYBACK_API BLUEPLAYBACK_HANDLE BluePlaybackCreate()
	{
		int DevCount = 0;
		CPlaybackDevice * pBluePlayback = NULL;
		pBluePlayback = new CPlaybackDevice();

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

	BLUEPLAYBACK_API int BluePlaybackRender(BLUEPLAYBACK_HANDLE pBPHandle, BYTE* pBuffer)
	{
		CPlaybackDevice * pBluePlayback = (CPlaybackDevice *)pBPHandle;
		if(!pBluePlayback || !pBuffer)
			return -1;

		pBluePlayback->Render(pBuffer);
		return 0;
	}

	BLUEPLAYBACK_API void BluePlaybackDestroy(BLUEPLAYBACK_HANDLE pBPHandle)
	{
		CPlaybackDevice * pBluePlayback = (CPlaybackDevice *)pBPHandle;

		if(pBluePlayback) {
			pBluePlayback->Stop();
			delete pBluePlayback;
		}
	}


	BLUEPLAYBACK_API int BluePlaybackSetCardProperty(BLUEPLAYBACK_HANDLE pBPHandle, int nProp,unsigned int nValue)
	{
		CPlaybackDevice * pBluePlayback = (CPlaybackDevice *)pBPHandle;

		if(!pBluePlayback)
			return -1;

		return pBluePlayback->SetCardProperty(nProp, nValue);
	}

	BLUEPLAYBACK_API int BluePlaybackSetCardPropertyInt64(BLUEPLAYBACK_HANDLE pBPHandle, int nProp,__int64 nValue)
	{
		CPlaybackDevice * pBluePlayback = (CPlaybackDevice *)pBPHandle;

		if(!pBluePlayback)
			return -1;

		return pBluePlayback->SetCardProperty(nProp, nValue);
	}

	BLUEPLAYBACK_API ULONG BluePlaybackGetMemorySize(BLUEPLAYBACK_HANDLE pBPHandle)
	{
		CPlaybackDevice * pBluePlayback = (CPlaybackDevice *)pBPHandle;

		if(!pBluePlayback)
			return -1;

		return pBluePlayback->GetMemorySize();
	}
}	//end of extern "C"
