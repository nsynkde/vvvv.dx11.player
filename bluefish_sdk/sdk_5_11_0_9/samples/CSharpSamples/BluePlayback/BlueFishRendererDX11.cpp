#include "stdafx.h"
#include "BlueFishRendererDX11.h"

BlueFishRendererDX11::BlueFishRendererDX11(HANDLE tex_handle, EMemoryFormat outFormat)
	:m_ReadTexture(new ReadTextureDX11(tex_handle,outFormat))
	,m_PlaybackDevice((CPlaybackDevice*)BluePlaybackCreate())
	,m_DroppedFrames(0)
	,m_Stopping(false)
{
}


CPlaybackDevice * BlueFishRendererDX11::GetDevice()
{
	return m_PlaybackDevice.get();
}

void BlueFishRendererDX11::OnPresent()
{
	if(m_Stopping) return;

	auto playbackDevice = m_PlaybackDevice;
	m_ThreadPresent.add_work([playbackDevice]{
		playbackDevice->RenderNext();
	});

	auto readTexture = m_ReadTexture.get();
	m_ThreadRender.add_work([playbackDevice,readTexture]{
		auto memory = readTexture->ReadBack();
        if (memory != nullptr)
        {
            playbackDevice->Upload((BYTE*)memory);
        }
	});
}

double BlueFishRendererDX11::GetAvgDurationMS()
{
	return double (std::chrono::duration_cast<std::chrono::microseconds>(m_ThreadRender.get_work_avg_duration()).count() + 
		std::chrono::duration_cast<std::chrono::microseconds>(m_ThreadPresent.get_work_avg_duration()).count()) / 1000.0;
}

double BlueFishRendererDX11::GetMaxDurationMS()
{
	return double (std::chrono::duration_cast<std::chrono::microseconds>(m_ThreadRender.get_work_max_duration()).count() + 
		std::chrono::duration_cast<std::chrono::microseconds>(m_ThreadPresent.get_work_max_duration()).count()) / 1000.0;
}

void BlueFishRendererDX11::Stop()
{
	m_Stopping = true;
	/*OutputDebugStringA("Stopping");
	OutputDebugStringA("Stopping present");
	m_ThreadPresent.stop();
	OutputDebugStringA("Stopping render");
	m_ThreadRender.stop();
	/*OutputDebugStringA("Joining present");
	m_ThreadPresent.join();
	OutputDebugStringA("Joining render");
	m_ThreadRender.join();
	OutputDebugStringA("Stopping device");
	m_PlaybackDevice->Stop();*/
}