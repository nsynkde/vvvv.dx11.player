#include <windows.h>
#include <stdio.h>

#include "stdafx.h"
#include "PlaybackDevice.h"

using namespace std;




/***************************************************************
bool SaveBMP ( BYTE* Buffer, int width, int height, 
		long paddedsize, LPCTSTR bmpfile )

Function takes a buffer of size <paddedsize> 
and saves it as a <width> * <height> sized bitmap 
under the supplied filename.
On error the return value is false.

***************************************************************/

bool SaveBMP ( BYTE* Buffer, int width, int height, long paddedsize, LPCTSTR bmpfile )
{
	// declare bmp structures 
	BITMAPFILEHEADER bmfh;
	BITMAPINFOHEADER info;
	
	// andinitialize them to zero
	memset ( &bmfh, 0, sizeof (BITMAPFILEHEADER ) );
	memset ( &info, 0, sizeof (BITMAPINFOHEADER ) );
	
	// fill the fileheader with data
	bmfh.bfType = 0x4d42;       // 0x4d42 = 'BM'
	bmfh.bfReserved1 = 0;
	bmfh.bfReserved2 = 0;
	bmfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + paddedsize;
	bmfh.bfOffBits = 0x36;		// number of bytes to start of bitmap bits
	
	// fill the infoheader

	info.biSize = sizeof(BITMAPINFOHEADER);
	info.biWidth = width;
	info.biHeight = height;
	info.biPlanes = 1;			// we only have one bitplane
	info.biBitCount = 24;		// RGB mode is 24 bits
	info.biCompression = BI_RGB;	
	info.biSizeImage = 0;		// can be 0 for 24 bit images
	info.biXPelsPerMeter = 0x0ec4;     // paint and PSP use this values
	info.biYPelsPerMeter = 0x0ec4;     
	info.biClrUsed = 0;			// we are in RGB mode and have no palette
	info.biClrImportant = 0;    // all colors are important

	// now we open the file to write to	
	HANDLE file = CreateFile ( bmpfile , GENERIC_WRITE, FILE_SHARE_READ,
		 NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );

	if ( file == NULL )
	{
		printf("file == NULL\n");
		CloseHandle ( file );
		return false;
	}
	
	// write file header
	unsigned long bwritten;
	if ( WriteFile ( file, &bmfh, sizeof ( BITMAPFILEHEADER ), &bwritten, NULL ) == false )
	{	
		printf("could not write header\n");
		CloseHandle ( file );
		return false;
	}
	// write infoheader
	if ( WriteFile ( file, &info, sizeof ( BITMAPINFOHEADER ), &bwritten, NULL ) == false )
	{	
		printf("could not write info\n");
		CloseHandle ( file );
		return false;
	}
	// write image data
	if ( WriteFile ( file, Buffer, paddedsize, &bwritten, NULL ) == false )
	{	
		printf("could not write data\n");
		CloseHandle ( file );
		return false;
	}
	
	// and close file
	CloseHandle ( file );

	return true;
}



CPlaybackDevice::CPlaybackDevice()
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

	OutputDebugString(L"CPlaybackDevice");
}

CPlaybackDevice::~CPlaybackDevice()
{
	if(m_pSDK)
	{
		if(m_DeviceID != -1)
			m_pSDK->device_detach();

		BlueVelvetDestroy(m_pSDK);
		m_pSDK = NULL;
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
	if(m_DeviceID == -1 && m_pSDK)
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
			err = SetCardProperty(DEFAULT_VIDEO_OUTPUT_CHANNEL, (UINT32)inChannel);
			m_VideoChannel = inChannel;
			swprintf_s(DebugString, 256, L"channel %d", err);
			OutputDebugString(DebugString);
		}

		//route output channel
		//RouteChannel(pSDK, EPOCH_SRC_OUTPUT_MEM_INTERFACE_CHA, EPOCH_DEST_SDI_OUTPUT_A, BLUE_CONNECTOR_PROP_SINGLE_LINK);
		if(err == 0)
		{
			VARIANT varVal;
			varVal.vt = VT_UI4;

			varVal.ulVal = EPOCH_SET_ROUTING(EPOCH_SRC_OUTPUT_MEM_INTERFACE_CHA, EPOCH_DEST_SDI_OUTPUT_A, BLUE_CONNECTOR_PROP_SINGLE_LINK);
			err = m_pSDK->SetCardProperty(MR2_ROUTING, varVal);
			swprintf_s(DebugString, 256, L"routing %d", err);
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
			OverlapChA.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

			// get video sizes
			ULONG GoldenSize = BlueVelvetGolden((ULONG)inVidFmt, (ULONG)inMemFmt, (ULONG)inUpdFmt);
			m_PixelsPerLine = BlueVelvetLinePixels((ULONG)inVidFmt);
			m_VideoLines =  BlueVelvetFrameLines((ULONG)inVidFmt, (ULONG)inUpdFmt);
			m_BytesPerFrame = BlueVelvetFrameBytes((ULONG)inVidFmt, (ULONG)inMemFmt, (ULONG)inUpdFmt);
			m_BytesPerLine = BlueVelvetLineBytes((ULONG)inVidFmt, (ULONG)inMemFmt);


			printf("Video Golden:          %ld\n", GoldenSize);
			printf("Video Pixels per line: %ld\n", m_PixelsPerLine);
			printf("Video lines:           %ld\n", m_VideoLines);
			printf("Video Bytes per frame: %ld\n", m_BytesPerFrame);
			printf("Video Bytes per line:  %ld\n", m_BytesPerLine);
			
			

			//m_Buffersize = 720*576*4;
			m_Buffersize = GoldenSize;
		}

		if(err == 0)
			return err;
	}
	return false;
}

int CPlaybackDevice::Start()
{
	return 0;
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
	

	CloseHandle(OverlapChA.hEvent);

	return 0;
}

void CPlaybackDevice::Render(BYTE* pBuffer)
{
	unsigned long fieldcount = 0;

	if(!pBuffer || !m_pSDK)
		return;


	//printf("writing bytes @ %d: size: %ld to Buffer: %d\n", pBuffer, m_Buffersize, m_ActiveBuffer);
#ifdef WRITE_BMP
	if (!SaveBMP(pBuffer, (int)m_PixelsPerLine, (int)m_VideoLines, m_BytesPerFrame, L"C:\\Users\\nsynk\\Desktop\\test.bmp"))
	{
		printf("error writing to file\n");
	}
#endif

	void* pAddressNotUsedChA = NULL;
	ULONG BufferIdChA = 0;
	ULONG UnderrunChA = 0;
	ULONG UniqueIdChA = 0;
	ULONG LastUnderrunChA = 0;
	DWORD BytesReturnedChA = 0;

	if(!(m_pSDK->video_playback_allocate(&pAddressNotUsedChA, BufferIdChA, UnderrunChA)))
	{
		//m_pSDK->system_buffer_write_async((unsigned char *)pBuffer, m_Buffersize, NULL, m_ActiveBuffer, 0);
		m_pSDK->system_buffer_write_async((unsigned char *)pBuffer, m_Buffersize, NULL, BlueImage_DMABuffer(BufferIdChA, BLUE_DATA_FRAME), 0);

		m_ActiveBuffer++;
		m_ActiveBuffer %= 2;

		//wait for both DMA transfers to be finished
			GetOverlappedResult(m_pSDK->m_hDevice, &OverlapChA, &BytesReturnedChA, TRUE);
			ResetEvent(OverlapChA.hEvent);

			//tell the card to playback the frames on the next interrupt
			m_pSDK->video_playback_present(UniqueIdChA, BlueBuffer_Image(BufferIdChA), 1, 0);

			//track UnderrunChA and UnderrunChB to see if frames were dropped
			if(UnderrunChA != LastUnderrunChA)
			{
				printf("Dropped a frame: ChA underruns: %ld\n", UnderrunChA);
				LastUnderrunChA = UnderrunChA;
			}

	}
	else
	{
			m_pSDK->wait_output_video_synch(UPD_FMT_FRAME, fieldcount);
	}
	//m_pSDK->wait_output_video_synch(UPD_FMT_FRAME, fieldcount);
	//m_pSDK->system_buffer_write_async((unsigned char *)pBuffer, m_Buffersize, NULL, m_ActiveBuffer, 0);
	//m_pSDK->render_buffer_update(m_ActiveBuffer);

	//m_ActiveBuffer++;
	//m_ActiveBuffer %= 2;
}

int CPlaybackDevice::SetCardProperty(int nProp,unsigned int nValue)
{
	int error_code;
	VARIANT objVar;
	objVar.vt = VT_UI4;
	objVar.ulVal = nValue;
	error_code = m_pSDK->SetCardProperty(nProp,objVar);
	return error_code;
}

int CPlaybackDevice::SetCardProperty(int nProp,__int64 nValue)
{
	int error_code;
	VARIANT objVar;
	objVar.vt = VT_UI8;
	objVar.ullVal = nValue;
	error_code = m_pSDK->SetCardProperty(nProp,objVar);
	return error_code;
}

