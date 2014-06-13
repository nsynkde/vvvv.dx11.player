// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the BLUEPLAYBACK_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// BLUEPLAYBACK_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef BLUEPLAYBACK_EXPORTS
#define BLUEPLAYBACK_API __declspec(dllexport)
#else
#define BLUEPLAYBACK_API __declspec(dllimport)
#endif

typedef void * BLUEPLAYBACK_HANDLE;

extern "C"
{
	BLUEPLAYBACK_API BLUEPLAYBACK_HANDLE BluePlaybackCreate();
	BLUEPLAYBACK_API int BluePlaybackConfig(BLUEPLAYBACK_HANDLE pBPHandle, INT32 inDevNo,
																			INT32 inChannel,
																			INT32 inVidFmt,
																			INT32 inMemFmt,
																			INT32 inUpdFmt,
																			INT32 inVideoDestination,
																			INT32 inAudioDestination,
																			INT32 inAudioChannelMask);

	BLUEPLAYBACK_API int BluePlaybackStart(BLUEPLAYBACK_HANDLE pBPHandle);
	BLUEPLAYBACK_API int BluePlaybackStop(BLUEPLAYBACK_HANDLE pBPHandle);

	BLUEPLAYBACK_API int BluePlaybackRender(BLUEPLAYBACK_HANDLE pBPHandle, BYTE* pBuffer);

	BLUEPLAYBACK_API void BluePlaybackDestroy(BLUEPLAYBACK_HANDLE pBPHandle);

	BLUEPLAYBACK_API int BluePlaybackSetCardProperty(BLUEPLAYBACK_HANDLE pBPHandle, int nProp, unsigned int nValue);
	BLUEPLAYBACK_API int BluePlaybackSetCardPropertyInt64(BLUEPLAYBACK_HANDLE pBPHandle, int nProp, __int64 nValue);

	BLUEPLAYBACK_API int BluePlaybackQueryCardProperty(BLUEPLAYBACK_HANDLE pBPHandle, int nProp);

	BLUEPLAYBACK_API ULONG BluePlaybackGetMemorySize(BLUEPLAYBACK_HANDLE pBPHandle);

	BLUEPLAYBACK_API void BluePlaybackGetSerialNumber(BLUEPLAYBACK_HANDLE pBPHandle);

	BLUEPLAYBACK_API const char* BluePlaybackBlueVelvetVersion();

	BLUEPLAYBACK_API int BluePlaybackGetDeviceCount(BLUEPLAYBACK_HANDLE pBPHandle);
}
