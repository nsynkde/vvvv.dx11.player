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
typedef void * BLUERENDERER_HANDLE;
typedef void * DX11_HANDLE;

extern "C"
{
	BLUEPLAYBACK_API BLUEPLAYBACK_HANDLE BluePlaybackCreate(int num_bluefish_buffers);
	BLUEPLAYBACK_API int BluePlaybackConfig(BLUEPLAYBACK_HANDLE pBPHandle, INT32 inDevNo,
																			INT32 inChannel,
																			INT32 inVidFmt,
																			INT32 inMemFmt,
																			INT32 inUpdFmt,
																			INT32 inVideoDestination,
																			INT32 inAudioDestination,
																			INT32 inAudioChannelMask);

	BLUEPLAYBACK_API int BluePlaybackStart(BLUEPLAYBACK_HANDLE pBPHandle);
	//BLUEPLAYBACK_API int BluePlaybackStop(BLUEPLAYBACK_HANDLE pBPHandle);

	BLUEPLAYBACK_API int BluePlaybackWaitSync(BLUEPLAYBACK_HANDLE pBPHandle);
	/*BLUEPLAYBACK_API int BluePlaybackUpload(BLUEPLAYBACK_HANDLE pBPHandle, BYTE* pBuffer);
	BLUEPLAYBACK_API int BluePlaybackRenderNext(BLUEPLAYBACK_HANDLE pBPHandle);*/

	//BLUEPLAYBACK_API void BluePlaybackDestroy(BLUEPLAYBACK_HANDLE pBPHandle);

	BLUEPLAYBACK_API int BluePlaybackSetCardProperty(BLUEPLAYBACK_HANDLE pBPHandle, int nProp, unsigned int nValue);
	BLUEPLAYBACK_API int BluePlaybackSetCardPropertyInt64(BLUEPLAYBACK_HANDLE pBPHandle, int nProp, __int64 nValue);

	BLUEPLAYBACK_API int BluePlaybackQueryUIntCardProperty(BLUEPLAYBACK_HANDLE pBPHandle, int nProp, UINT32 & outValue);
	BLUEPLAYBACK_API int BluePlaybackQueryULongCardProperty(BLUEPLAYBACK_HANDLE pBPHandle, int nProp, ULONG & outValue);

	BLUEPLAYBACK_API ULONG BluePlaybackGetMemorySize(BLUEPLAYBACK_HANDLE pBPHandle);

	BLUEPLAYBACK_API const char* BluePlaybackGetSerialNumber(BLUEPLAYBACK_HANDLE pBPHandle);

	BLUEPLAYBACK_API const char* BluePlaybackBlueVelvetVersion();

	BLUEPLAYBACK_API int BluePlaybackGetDeviceCount(BLUEPLAYBACK_HANDLE pBPHandle);

	BLUEPLAYBACK_API unsigned long BluePlaybackGetPixelsPerLine(BLUEPLAYBACK_HANDLE pBPHandle);
	BLUEPLAYBACK_API unsigned long BluePlaybackGetVideoLines(BLUEPLAYBACK_HANDLE pBPHandle);
	BLUEPLAYBACK_API unsigned long BluePlaybackGetBytesPerLine(BLUEPLAYBACK_HANDLE pBPHandle);
	BLUEPLAYBACK_API unsigned long BluePlaybackGetBytesPerFrame(BLUEPLAYBACK_HANDLE pBPHandle);


	BLUEPLAYBACK_API BLUERENDERER_HANDLE BlueRendererCreate(HANDLE tex_handle, int outFormat, int num_render_target_buffers, int num_read_back_buffers, int num_bluefish_buffers);
	BLUEPLAYBACK_API void BlueRendererDestroy(BLUERENDERER_HANDLE renderer);
	BLUEPLAYBACK_API void BlueRendererStop(BLUERENDERER_HANDLE pBPHandle);
	BLUEPLAYBACK_API void BlueRendererSetSharedHandle(BLUERENDERER_HANDLE renderer, HANDLE tex_handle);
	BLUEPLAYBACK_API BLUEPLAYBACK_HANDLE BlueRendererGetPlaybackDevice(BLUERENDERER_HANDLE renderer);
	BLUEPLAYBACK_API DX11_HANDLE BlueRendererGetDX11Device(BLUERENDERER_HANDLE renderer);
	BLUEPLAYBACK_API void BlueRendererOnPresent(BLUERENDERER_HANDLE renderer);
	BLUEPLAYBACK_API double BlueRendererGetAvgDuration(BLUERENDERER_HANDLE renderer);
	BLUEPLAYBACK_API double BlueRendererGetMaxDuration(BLUERENDERER_HANDLE renderer);
}
