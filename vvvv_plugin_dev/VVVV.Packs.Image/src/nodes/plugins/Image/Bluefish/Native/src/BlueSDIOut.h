#ifndef _PLAYBACK_SDI_OUT_H
#define _PLAYBACK_SDI_OUT_H

#include "BlueVelvet4.h"

class CPlaybackSDIOut
{
	
public:
	CBlueVelvet4*	m_pSDK;

protected:
	int				m_DeviceID;
	int				m_SDIOut;

public:
	
	CPlaybackSDIOut(int device, int sdi_out);
	void Route(int memory_channel);
	void SetTransport(EHdSdiTransport transport);
};

#endif