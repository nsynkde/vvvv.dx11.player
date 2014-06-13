//
//  Copyright (c) 2014 Bluefish444. All rights reserved.
//

#ifndef HG_BLUEVELVET_C
#define HG_BLUEVELVET_C


#ifdef WIN32
#include "BlueVelvet4.h"
#include "BlueHancUtils.h"

    #ifdef BLUEVELVETC_EXPORTS
        #define BLUEVELVETC_API __declspec(dllexport)
    #else
        #define BLUEVELVETC_API __declspec(dllimport)
    #endif
#elif defined __APPLE__
    #define DLLEXPORT __attribute__ ((visibility("default")))
    #define DLLLOCAL __attribute__ ((visibility("hidden")))
    #define BLUEVELVETC_API DLLEXPORT

    #include "BlueDriver_p.h"
    #include "BlueEvent.h"
    #define OVERLAPPED BlueEvent
#endif



typedef void* BLUEVELVETC_HANDLE;


#ifdef WIN32
extern "C" {
#endif

	BLUEVELVETC_API const char* bfcGetVersion();
	BLUEVELVETC_API BLUEVELVETC_HANDLE bfcFactory();
	BLUEVELVETC_API void bfcDestroy(BLUEVELVETC_HANDLE pHandle);

	BLUEVELVETC_API int bfcEnumerate(BLUEVELVETC_HANDLE pHandle, int& iDevices);
#ifdef WIN32
	BLUEVELVETC_API int bfcQueryCardType(BLUEVELVETC_HANDLE pHandle, int iDeviceID=0);
	BLUEVELVETC_API int bfcAttach(BLUEVELVETC_HANDLE pHandle, int& iDeviceId);
	BLUEVELVETC_API int bfcDetach(BLUEVELVETC_HANDLE pHandle);

	BLUEVELVETC_API int bfcQueryCardProperty32(BLUEVELVETC_HANDLE pHandle, int iProperty, unsigned int& nValue);
	BLUEVELVETC_API int bfcSetCardProperty32(BLUEVELVETC_HANDLE pHandle, int iProperty, unsigned int& nValue);
	BLUEVELVETC_API int bfcQueryCardProperty64(BLUEVELVETC_HANDLE pHandle, int iProperty, unsigned long long& nValue);
	BLUEVELVETC_API int bfcSetCardProperty64(BLUEVELVETC_HANDLE pHandle, int iProperty, unsigned long long& nValue);
#elif defined __APPLE__
	BLUEVELVETC_API int bfcQueryCardType(BLUEVELVETC_HANDLE pHandle, int& iCardType, int iDeviceID = 0);
	BLUEVELVETC_API int bfcAttach(BLUEVELVETC_HANDLE pHandle, int iDeviceId);
	BLUEVELVETC_API int bfcDetach(BLUEVELVETC_HANDLE pHandle);
  
	BLUEVELVETC_API int bfcQueryCardProperty32(BLUEVELVETC_HANDLE pHandle, const int iProperty, unsigned int& nValue);
	BLUEVELVETC_API int bfcSetCardProperty32(BLUEVELVETC_HANDLE pHandle, const int iProperty, unsigned int nValue);
	BLUEVELVETC_API int bfcQueryCardProperty64(BLUEVELVETC_HANDLE pHandle, const int iProperty, unsigned long long& nValue);
	BLUEVELVETC_API int bfcSetCardProperty64(BLUEVELVETC_HANDLE pHandle, const int iProperty, unsigned long long nValue);
 #endif  

	BLUEVELVETC_API int bfcGetCardSerialNumber(BLUEVELVETC_HANDLE pHandle, char* pSerialNumber, unsigned int nStringSize); //nStringSize must be at least 20

	BLUEVELVETC_API int bfcVideoCaptureStart(BLUEVELVETC_HANDLE pHandle);
	BLUEVELVETC_API int bfcVideoCaptureStop(BLUEVELVETC_HANDLE pHandle);

	BLUEVELVETC_API int bfcVideoPlaybackStart(BLUEVELVETC_HANDLE pHandle, int iStep, int iLoop);
	BLUEVELVETC_API int bfcVideoPlaybackStop(BLUEVELVETC_HANDLE pHandle, int iWait, int iFlush);

	BLUEVELVETC_API int bfcWaitVideoInputSync(BLUEVELVETC_HANDLE pHandle, unsigned long ulUpdateType, unsigned long& ulFieldCount);
	BLUEVELVETC_API int bfcWaitVideoOutputSync(BLUEVELVETC_HANDLE pHandle, unsigned long ulUpdateType, unsigned long& ulFieldCount);
	BLUEVELVETC_API int bfcWaitVideoSyncAsync(BLUEVELVETC_HANDLE pHandle, OVERLAPPED* pOverlap, blue_video_sync_struct* pSyncData);

	BLUEVELVETC_API int bfcGetVideoPlaybackCurrentFieldCount(BLUEVELVETC_HANDLE pHandle, unsigned long& ulFieldCount);
    BLUEVELVETC_API int bfcGetVideoCaptureCurrentFieldCount(BLUEVELVETC_HANDLE pHandle, unsigned long& ulFieldCount);
	BLUEVELVETC_API int bfcSystemBufferReadAsync(BLUEVELVETC_HANDLE pHandle, unsigned char* pPixels, unsigned long ulSize, OVERLAPPED* pOverlap, unsigned long ulBufferID, unsigned long ulOffset=0);
	BLUEVELVETC_API int bfcSystemBufferWriteAsync(BLUEVELVETC_HANDLE pHandle, unsigned char* pPixels, unsigned long ulSize, OVERLAPPED* pOverlap, unsigned long ulBufferID, unsigned long ulOffset=0);

	BLUEVELVETC_API int bfcGetCaptureVideoFrameInfoEx(BLUEVELVETC_HANDLE pHandle, OVERLAPPED* pOverlap, struct blue_videoframe_info_ex& VideoFrameInfo, int iCompostLater, unsigned int* nCaptureFifoSize);
	BLUEVELVETC_API int bfcVideoPlaybackAllocate(BLUEVELVETC_HANDLE pHandle, void** pAddress, unsigned long& ulBufferID, unsigned long& ulUnderrun);
	BLUEVELVETC_API int bfcVideoPlaybackPresent(BLUEVELVETC_HANDLE pHandle, unsigned long& ulUniqueID, unsigned long ulBufferID, unsigned long ulCount, int iKeep, int iOdd=0);
	BLUEVELVETC_API int bfcVideoPlaybackRelease(BLUEVELVETC_HANDLE pHandle, unsigned long ulBufferID);

	BLUEVELVETC_API int bfcRenderBufferCapture(BLUEVELVETC_HANDLE pHandle, unsigned long ulBufferID);
	BLUEVELVETC_API int bfcRenderBufferUpdate(BLUEVELVETC_HANDLE pHandle, unsigned long ulBufferID);

	//AUDIO Helper functions (BlueHancUtils)
	BLUEVELVETC_API int bfcEncodeHancFrameEx(BLUEVELVETC_HANDLE pHandle, unsigned int nCardType, struct hanc_stream_info_struct* pHancEncodeInfo, void* pAudioBuffer, unsigned int nAudioChannels, unsigned int nAudioSamples, unsigned int nSampleType, unsigned int nAudioFlags);
	BLUEVELVETC_API int bfcDecodeHancFrameEx(BLUEVELVETC_HANDLE pHandle, unsigned int nCardType, unsigned int* pHancBuffer, struct hanc_decode_struct* pHancDecodeInfo);

	//Miscellaneous functions
	BLUEVELVETC_API int bfcGetReferenceClockPhaseSettings(BLUEVELVETC_HANDLE pHandle, unsigned int& nHPhase, unsigned int& nVPhase, unsigned int& nHPhaseMax, unsigned int& nVPhaseMax);
	BLUEVELVETC_API int bfcLoadOutputLUT1D(BLUEVELVETC_HANDLE pHandle, struct blue_1d_lookup_table_struct* pLutData);
	BLUEVELVETC_API int bfcControlVideoScaler(BLUEVELVETC_HANDLE pHandle,	unsigned int nScalerId,
																			bool bOnlyReadValue,
																			float* pSrcVideoHeight,
																			float* pSrcVideoWidth,
																			float* pSrcVideoYPos,
																			float* pSrcVideoXPos,
																			float* pDestVideoHeight,
																			float* pDestVideoWidth,
																			float* pDestVideoYPos,
																			float* pDestVideoXPos);
	// Video mode and Format information functions
	BLUEVELVETC_API int bfcGetPixelsPerLine(EVideoMode nVideoMode, unsigned int& nPixelsPerLine);
    BLUEVELVETC_API int bfcGetLinesPerFrame(EVideoMode nVideoMode, unsigned int& nLinesPerFrame);
    BLUEVELVETC_API int bfcGetBytesPerLine(EVideoMode nVideoMode, EMemoryFormat nMemoryFormat, unsigned int& nBytesPerLine);
    BLUEVELVETC_API int bfcGetBytesPerFrame(EVideoMode nVideoMode, EMemoryFormat nMemoryFormat, EUpdateMethod nUpdateMethod, unsigned int& nBytesPerFrame);
    BLUEVELVETC_API int bfcGetGoldenValue(EVideoMode nVideoMode, EMemoryFormat nMemoryFormat, EUpdateMethod nUpdateMethod, unsigned int& nGoldenFrameSize);
    BLUEVELVETC_API int bfcGetVBILines(EVideoMode nVideoMode, EUpdateMethod nUpdateMethod, unsigned int& nVBILinesPerFrame);
    

#ifdef WIN32	
	//AMD SDI-Link support
	BLUEVELVETC_API int bfcDmaWaitMarker(BLUEVELVETC_HANDLE pHandle,	OVERLAPPED* pOverlap,
																	unsigned int nVideoChannel,
																	unsigned int nBufferId,
																	unsigned int nMarker);
	BLUEVELVETC_API int bfcDmaReadToBusAddressWithMarker(	BLUEVELVETC_HANDLE pHandle,
															unsigned int nVideoChannel,
															unsigned long long n64DataAddress,
															unsigned int nSize,
															OVERLAPPED* pOverlap,
															unsigned int BufferID,
															unsigned long Offset,
															unsigned long long n64MarkerAddress,
															unsigned int nMarker);
	BLUEVELVETC_API int bfcDmaReadToBusAddress(	BLUEVELVETC_HANDLE pHandle,
												unsigned int nVideoChannel,
												unsigned long long n64DataAddress,
												unsigned int nSize,
												OVERLAPPED* pOverlap,
												unsigned int BufferID,
												unsigned long Offset);
	BLUEVELVETC_API int bfcDmaWriteMarker(	BLUEVELVETC_HANDLE pHandle,
											unsigned long long n64MarkerAddress,
											unsigned int nMarker);
#endif


#ifdef WIN32
} //extern "C"
#endif

#endif //HG_BLUEVELVET_C
