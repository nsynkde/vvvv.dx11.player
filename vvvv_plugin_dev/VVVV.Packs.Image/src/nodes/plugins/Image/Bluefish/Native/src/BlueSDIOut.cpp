#include "stdafx.h"
#include "BlueSDIOut.h"
#include "BlueVelvet4.h"

CPlaybackSDIOut::CPlaybackSDIOut(int device, int sdi_out)
{
	m_pSDK = BlueVelvetFactory4();
	m_DeviceID = device;
	m_SDIOut = sdi_out;
	m_pSDK->device_attach(device,0);
}

void CPlaybackSDIOut::Route(int memory_channel)
{
	VARIANT varVal;
	int err;
	varVal.vt = VT_UI4;
	int out_memory_channel;
	switch(memory_channel){
	case 0:
		out_memory_channel = EPOCH_SRC_OUTPUT_MEM_INTERFACE_CHA;
		break;
	case 1:
		out_memory_channel = EPOCH_SRC_OUTPUT_MEM_INTERFACE_CHB;
		break;
	case 2:
		out_memory_channel = EPOCH_SRC_OUTPUT_MEM_INTERFACE_CHC;
		break;
	case 3:
		out_memory_channel = EPOCH_SRC_OUTPUT_MEM_INTERFACE_CHD;
		break;
	}
	varVal.ulVal = EPOCH_SET_ROUTING(out_memory_channel, EPOCH_DEST_SDI_OUTPUT_A + m_SDIOut, BLUE_CONNECTOR_PROP_SINGLE_LINK);
	err = m_pSDK->SetCardProperty(MR2_ROUTING, varVal);
	VARIANT objVar;
	objVar.vt = VT_UI4;
	objVar.ulVal = memory_channel;
	err = m_pSDK->SetCardProperty(DEFAULT_VIDEO_OUTPUT_CHANNEL,objVar);
}

void CPlaybackSDIOut::SetTransport(EHdSdiTransport transport)
{
	int error_code;
	VARIANT objVar;
	objVar.vt = VT_UI4;
	objVar.ulVal = transport;
	objVar.ulVal |= (BLUE_CONNECTOR_SDI_OUTPUT_A + m_SDIOut) << 16;
	error_code = m_pSDK->SetCardProperty(EPOCH_HD_SDI_TRANSPORT,objVar);
}
