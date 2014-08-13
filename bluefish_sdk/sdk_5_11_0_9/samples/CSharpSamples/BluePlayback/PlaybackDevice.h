#ifndef _PLAYBACK_DEVICE_H
#define _PLAYBACK_DEVICE_H

#include "BlueVelvet4.h"

class CPlaybackDevice
{
public:
	CBlueVelvet4*	m_pSDK;

protected:
	int				m_DeviceID;
	int				m_CardType;
	int				m_VideoChannel;
	unsigned int	m_VidFmt;
	unsigned int	m_MemFmt;
	unsigned int	m_UpdFmt;
	unsigned long	m_Buffersize;
	int				m_ActiveBuffer;
	int				m_FrontBuffer;
	unsigned long	m_PixelsPerLine;
	unsigned long	m_VideoLines;
	unsigned long	m_BytesPerFrame;
	unsigned long	m_BytesPerLine;
	
	OVERLAPPED * OverlapChA;
	bool * ready;

	void RefreshProperties();

public:
	CPlaybackDevice();
	~CPlaybackDevice();

	int CheckCardPresent();
	int	Config(INT32 inDevNo, INT32 inChannel,
				INT32 inVidFmt, INT32 inMemFmt, INT32 inUpdFmt,
				INT32 inVideoDestination, INT32 inAudioDestination, INT32 inAudioChannelMask);
	int Start();
	int Stop();
	void WaitSync();
	void Upload(BYTE* pBuffer);
	void RenderNext();

	int SetCardProperty(int nProp,unsigned int nValue);
	int SetCardProperty(int nProp,__int64 nValue);

	int QueryCardProperty(int nProp, VARIANT *varVal);

	unsigned long GetMemorySize() { return m_Buffersize; };

	int GetMemoryChannel();

	int GetCardSerialNumber(char* serial, int size);

	unsigned long GetPixelsPerLine();
	unsigned long GetVideoLines();
	unsigned long GetBytesPerLine();
	unsigned long GetBytesPerFrame();
};

#endif	//_PLAYBACK_DEVICE_H
