#include "stdafx.h"
#include "BlueFishRendererDX11.h"

BlueFishRendererDX11::BlueFishRendererDX11(HANDLE tex_handle, EMemoryFormat outFormat, int num_render_target_buffers, int num_read_back_buffers, int num_bluefish_buffers)
	:m_ReadTexture(new ReadTextureDX11(tex_handle,outFormat, num_render_target_buffers, num_read_back_buffers))
	,m_PlaybackDevice((CPlaybackDevice*)BluePlaybackCreate(num_bluefish_buffers))
	,m_DroppedFrames(0)
	,m_Stopping(false)
{
}


CPlaybackDevice * BlueFishRendererDX11::GetDevice()
{
	return m_PlaybackDevice.get();
}


ID3D11Device * BlueFishRendererDX11::GetDX11Device()
{
	return m_ReadTexture->GetDevice();
}


void BlueFishRendererDX11::SetSharedHandle(HANDLE tex_handle)
{
	m_ReadTexture->SetSharedHandle(tex_handle);
}

void BlueFishRendererDX11::OnPresent()
{
	if(m_Stopping) return;
	
	auto playbackDevice = m_PlaybackDevice;
	auto readTexture = m_ReadTexture;
	if(m_PlaybackDevice->RingBufferSize()<4){
		m_ThreadPresent.add_work([playbackDevice,readTexture]{
			playbackDevice->RenderNext();
			auto memory = readTexture->ReadBack();
			if (memory != nullptr)
			{
				playbackDevice->Upload((BYTE*)memory);
			}
		});
	}else{
		m_ThreadPresent.add_work([playbackDevice]{
			playbackDevice->RenderNext();
		});

		m_ThreadRender.add_work([playbackDevice,readTexture]{
			auto memory = readTexture->ReadBack();
			if (memory != nullptr)
			{
				playbackDevice->Upload((BYTE*)memory);
			}
		});
	}
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
	OutputDebugStringA("Stopping");

	m_Stopping = true;
	OutputDebugStringA("Stopping present");
	m_ThreadPresent.stop();
	OutputDebugStringA("Stopping render");
	m_ThreadRender.stop();
	
	OutputDebugStringA("Joining present");
	m_ThreadPresent.join();
	OutputDebugStringA("Joining render");
	m_ThreadRender.join();
	OutputDebugStringA("Stopping device");
	m_PlaybackDevice->Stop();
}