#include "PlaybackDevice.h"
#include "ReadTextureDX11.h"
#include <memory>
#include "WorkerThread.h"
#include "BluePlayback.h"


class BlueFishRendererDX11{
	std::shared_ptr<ReadTextureDX11> m_ReadTexture;
	std::shared_ptr<CPlaybackDevice> m_PlaybackDevice;
	WorkerThread m_ThreadPresent;
	WorkerThread m_ThreadRender;
	int m_DroppedFrames;
	bool m_Stopping;
public:
	BlueFishRendererDX11(HANDLE tex_handle, EMemoryFormat outFormat);

	CPlaybackDevice * GetDevice();

	void OnPresent();

	double GetAvgDurationMS();
	double GetMaxDurationMS();

	void Stop();
};
