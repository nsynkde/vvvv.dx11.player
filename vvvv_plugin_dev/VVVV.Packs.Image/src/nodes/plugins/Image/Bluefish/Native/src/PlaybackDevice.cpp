#include "stdafx.h"
#include "PlaybackDevice.h"
#include <sstream>


CPlaybackDevice::CPlaybackDevice(int numBuffers)
{
	m_pSDK = NULL;
	m_pSDK = BlueVelvetFactory4();
	m_DeviceID = -1;

	m_VideoChannel = BLUE_VIDEO_OUTPUT_CHANNEL_A;
	m_VidFmt = VID_FMT_INVALID;
	m_MemFmt = MEM_FMT_INVALID;
	m_UpdFmt = UPD_FMT_FRAME;
	m_CardType = CRD_INVALID;
	m_Buffersize = 0;
	m_ActiveBuffer = 0;
	m_FrontBuffer = numBuffers/2;
	m_NumBuffers = numBuffers;
	OverlapChA = new OVERLAPPED[m_NumBuffers];
	ready = new bool[m_NumBuffers];

	OutputDebugString(L"CPlaybackDevice");
}

CPlaybackDevice::~CPlaybackDevice()
{
	OutputDebugString(L"~CPlaybackDevice");
	if(m_pSDK)
	{
		Stop();
		if(m_DeviceID != -1)
			m_pSDK->device_detach();

		BlueVelvetDestroy(m_pSDK);
		m_pSDK = NULL;
		
		delete[] OverlapChA;
		delete[] ready;
	}	
}

int CPlaybackDevice::CheckCardPresent()
{
	int DevCount = 0;
	if(m_pSDK)
		m_pSDK->device_enumerate(DevCount);

	return DevCount;
}

int CPlaybackDevice::Config(INT32 inDevNo, INT32 inChannel,
							INT32 inVidFmt, INT32 inMemFmt, INT32 inUpdFmt,
							INT32 inVideoDestination, INT32 inAudioDestination, INT32 inAudioChannelMask)
{
	wchar_t DebugString[256];
	swprintf_s(DebugString, 256, L"Config: %d, %d, %d, %d, %d", inDevNo, inChannel, inVidFmt, inMemFmt, inUpdFmt);
	OutputDebugString(DebugString);

	int err = -1;
	if(m_pSDK)
	{
		//attach to the card
		err = m_pSDK->device_attach(inDevNo,0);
		if(err == 0)
		{
			m_DeviceID = inDevNo;
			m_CardType = (ECardType)m_pSDK->has_video_cardtype(inDevNo);
		}

		//set default video output channel (only for cards with more than 1 stream)
		if(err == 0 && m_CardType != CRD_BLUE_EPOCH_2K)
		{
			UINT32 out_memory_channel = BLUE_VIDEO_OUTPUT_CHANNEL_A;
			switch(inChannel){

			case 0:
				out_memory_channel = BLUE_VIDEO_OUTPUT_CHANNEL_A;
				break;
			case 1:
				out_memory_channel = BLUE_VIDEO_OUTPUT_CHANNEL_B;
				break;
			case 2:
				out_memory_channel = BLUE_VIDEO_OUTPUT_CHANNEL_C;
				break;
			case 3:
				out_memory_channel = BLUE_VIDEO_OUTPUT_CHANNEL_D;
				break;
			} 

			err = SetCardProperty(DEFAULT_VIDEO_OUTPUT_CHANNEL, out_memory_channel);
			m_VideoChannel = inChannel;
			swprintf_s(DebugString, 256, L"channel %d", err);
			OutputDebugString(DebugString);
		}


		//make sure the FIFO hasn't been left running (e.g. application crash before), otherwise we can't change card properties
		//m_pSDK->video_playback_stop(0, 0);


		//set the video mode
		if(err == 0)
		{
			err = SetCardProperty(VIDEO_MODE, (UINT32)inVidFmt);
			m_VidFmt = inVidFmt;
			swprintf_s(DebugString, 256, L"video mode %d", err);
			OutputDebugString(DebugString);
		}

		//set the video memory format
		if(err == 0)
		{
			err = SetCardProperty(VIDEO_MEMORY_FORMAT, (UINT32)inMemFmt);
			m_MemFmt = inMemFmt;
			swprintf_s(DebugString, 256, L"mem format %d", err);
			OutputDebugString(DebugString);
		}

		//set the video update type
		if(err == 0)
		{
			err = SetCardProperty(VIDEO_UPDATE_TYPE, (UINT32)inUpdFmt);
			m_UpdFmt = inUpdFmt;
			swprintf_s(DebugString, 256, L"upd type %d", err);
			OutputDebugString(DebugString);
		}


		//set video engine here??
		if(err == 0)
		{
			err = SetCardProperty(VIDEO_OUTPUT_ENGINE, (UINT32)VIDEO_ENGINE_FRAMESTORE);
			swprintf_s(DebugString, 256, L"engine type %d", err);
			OutputDebugString(DebugString);
		}

		


		//switch off black generator (enables video output)
		if(err == 0)
		{
			UINT32 iDummy = ENUM_BLACKGENERATOR_OFF;
			err = SetCardProperty(VIDEO_BLACKGENERATOR, iDummy);
			swprintf_s(DebugString, 256, L"enable output %d", err);
			OutputDebugString(DebugString);
		}
		
		if(err != 0)
		{
			swprintf_s(DebugString, 256, L"An error ocurred %d", err);
			OutputDebugString(DebugString);
		}

		if(err == 0)
		{
			for(int i=0;i<m_NumBuffers;i++){
				memset(&OverlapChA[i], 0, sizeof(OVERLAPPED));
				OverlapChA[i].hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
				ready[i] = false;
			}
			RefreshProperties();
		}
	}
	return err;
}

int CPlaybackDevice::Start()
{
	UINT32 iDummy = ENUM_BLACKGENERATOR_OFF;
	int err = SetCardProperty(VIDEO_BLACKGENERATOR, iDummy);
	return err;
}

int CPlaybackDevice::Stop()
{
	wchar_t DebugString[256];
	int err = -1;

	//switch on black generator (enables video output)
	UINT32 iDummy = ENUM_BLACKGENERATOR_ON;
	err = SetCardProperty(VIDEO_BLACKGENERATOR, iDummy);
	swprintf_s(DebugString, 256, L"enable output %d", err);
	OutputDebugString(DebugString);

	
	for(int i=0;i<m_NumBuffers;i++){
		CloseHandle(OverlapChA[i].hEvent);
	}

	return 0;
}

int CPlaybackDevice::GetMemoryChannel()
{
	return m_VideoChannel;
}

void CPlaybackDevice::WaitSync(){
	unsigned long fieldcount = 0;

	if (!m_pSDK)
		return;

	m_pSDK->wait_output_video_synch(UPD_FMT_FRAME, fieldcount);
}


void CPlaybackDevice::Upload(BYTE* pBuffer)
{
	if (!pBuffer || !m_pSDK)
		return;
	
	if(m_NumBuffers<4)
	{
		m_pSDK->system_buffer_write_async(pBuffer, m_Buffersize, NULL, m_ActiveBuffer, 0);
	}else{
		m_pSDK->system_buffer_write_async(pBuffer, m_Buffersize, &OverlapChA[m_ActiveBuffer], m_ActiveBuffer, 0);
	}
	ready[m_ActiveBuffer] = true;
	m_ActiveBuffer++;
	m_ActiveBuffer %= m_NumBuffers;
}

void CPlaybackDevice::RenderNext()
{
	if (!m_pSDK)
		return;
	
	if(m_NumBuffers<4)
	{
		if(ready[m_ActiveBuffer]){
			m_pSDK->render_buffer_update(m_ActiveBuffer);
		}
	}else{
		if(ready[m_FrontBuffer]){
			unsigned long	BytesReturnedChA;
			GetOverlappedResult(m_pSDK->m_hDevice, &OverlapChA[m_FrontBuffer], &BytesReturnedChA, TRUE);
			m_pSDK->render_buffer_update(m_FrontBuffer);
		}
		m_FrontBuffer++;
		m_FrontBuffer %= m_NumBuffers; 
	}

}

void CPlaybackDevice::RefreshProperties(){
	VARIANT inVidFmt;
	QueryCardProperty(VIDEO_MODE,&inVidFmt);
	VARIANT inMemFmt;
	QueryCardProperty(VIDEO_MEMORY_FORMAT,&inMemFmt);
	VARIANT inUpdFmt;
	QueryCardProperty(VIDEO_UPDATE_TYPE,&inUpdFmt);
	// get video sizes
	ULONG GoldenSize = BlueVelvetGolden(inVidFmt.ulVal, inMemFmt.ulVal, inUpdFmt.ulVal);
	m_PixelsPerLine = BlueVelvetLinePixels(inVidFmt.ulVal);
	m_VideoLines = BlueVelvetFrameLines(inVidFmt.ulVal, inUpdFmt.ulVal);
	m_BytesPerFrame = BlueVelvetFrameBytes(inVidFmt.ulVal, inMemFmt.ulVal, inUpdFmt.ulVal);
	m_BytesPerLine = BlueVelvetLineBytes(inVidFmt.ulVal, inMemFmt.ulVal);
	m_Buffersize = GoldenSize;
}

int CPlaybackDevice::SetCardProperty(int nProp,unsigned int nValue)
{
	int error_code;
	VARIANT objVar;
	objVar.vt = VT_UI4;
	objVar.ulVal = nValue;
	error_code = m_pSDK->SetCardProperty(nProp,objVar);
	RefreshProperties();
	return error_code;
}

int CPlaybackDevice::SetCardProperty(int nProp,__int64 nValue)
{
	int error_code;
	VARIANT objVar;
	objVar.vt = VT_UI8;
	objVar.ullVal = nValue;
	error_code = m_pSDK->SetCardProperty(nProp,objVar);
	RefreshProperties();
	return error_code;
}

int CPlaybackDevice::QueryCardProperty(int nProp, VARIANT *varVal)
{
	int error_code;
	error_code = m_pSDK->QueryCardProperty(nProp, *varVal);
	//cout << "Product ID / firmware type: " << *varVal.ulVal << endl;
	return error_code;
}

int CPlaybackDevice::GetCardSerialNumber(char* serialNum, int serialNumSize)
{
	int error_code;
	error_code = bfGetCardSerialNumber(m_pSDK, serialNum, serialNumSize); //nStringSize must be at least 20
	return error_code;
}

unsigned long CPlaybackDevice::GetPixelsPerLine()
{
	return m_PixelsPerLine;
}

unsigned long CPlaybackDevice::GetVideoLines()
{
	return m_VideoLines;
}

unsigned long CPlaybackDevice::GetBytesPerLine()
{
	return m_BytesPerLine;
}

unsigned long CPlaybackDevice::GetBytesPerFrame()
{
	return m_BytesPerFrame;
}

int CPlaybackDevice::RingBufferSize(){
	return m_NumBuffers;
}