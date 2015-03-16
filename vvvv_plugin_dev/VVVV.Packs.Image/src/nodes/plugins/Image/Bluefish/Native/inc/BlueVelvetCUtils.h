//
//  BlueVelvetCUtils.h
//  BlueVelvetC
//
//  Created by James on 21/05/2014.
//  Copyright (c) 2014 Bluefish444. All rights reserved.
//
// A file that contains useful Util functions that can ssist in general development.
// indente to be included as-is into BlueVelvetC projects and not part of a library.

#ifndef BlueVelvetC_BlueVelvetCUtils_h
#define BlueVelvetC_BlueVelvetCUtils_h

#include "BlueVelvetC.h"


#define TRACE_ERROR		1
#define TRACE_INFO		2
#define TRACE_DEBUG		3
#define	TRACE_FULL		4
#define TRACE_EXTREME	5
#define CURRENT_DEBUG_LEVEL TRACE_DEBUG


#if defined (_WIN32)
extern "C" {
#endif

// Logging functions
#ifndef _UNICODE
	BLUEVELVETC_API void			BLUE_DEBUG_LOG(int Level, const char* Format, ...);
#else
	BLUEVELVETC_API void			BLUE_DEBUG_LOG(int Level, const wchar_t* Format, ...);
#endif
	
// Memory Allocation and Free'ing of page Aligned memory
	BLUEVELVETC_API void*			bfAlloc(unsigned int nMemorySize);
	BLUEVELVETC_API void			bfFree(unsigned int nMemSize, void* pMemory);

// Get strings that give information about the card or card state
	BLUEVELVETC_API const char*		bfcUtilsGetStringForCardType(const int iCardType);
	BLUEVELVETC_API const wchar_t*	bfcUtilsGetWStringForCardType(const int iCardType);

	BLUEVELVETC_API const char*		bfcUtilsGetStringForBlueProductId(const unsigned int nProductId);
	BLUEVELVETC_API const wchar_t*	bfcUtilsGetWStringForBlueProductId(const unsigned int nProductId);

	BLUEVELVETC_API const char*		bfcUtilsGetStringForVideoFormat(const unsigned int vidFmt);
	BLUEVELVETC_API const wchar_t*	bfcUtilsGetWStringForVideoFormat(const unsigned int vidFmt);

	BLUEVELVETC_API const char*		bfcUtilsGetStringForMemoryFormat(const unsigned int memFmt);
	BLUEVELVETC_API const wchar_t*	bfcUtilsGetWStringForMemoryFormat(const unsigned int memFmt);			


// Get/Set MR2 Video mode info
	BLUEVELVETC_API int				bfcUtilsGetMR2VideoMode(const BLUEVELVETC_HANDLE pHandle, const unsigned int nMR2Node, unsigned int& nVideoMode);
	BLUEVELVETC_API int				bfcUtilsSetMR2VideoMode(const BLUEVELVETC_HANDLE pHandle, const unsigned int nMR2Node, unsigned int nVideoMode);

// Get/Set Audio routing.
	BLUEVELVETC_API int				bfcUtilsGetAudioOutputRouting(const BLUEVELVETC_HANDLE pHandle, const unsigned int audio_conn_type, unsigned int& pcm_audio_src_ch, unsigned int physical_audio_conn_ch);
	BLUEVELVETC_API int				bfcUtilsSetAudioOutputRouting(const BLUEVELVETC_HANDLE pHandle, const unsigned int audio_conn_type, unsigned int pcm_audio_src_ch, unsigned int physical_audio_conn_ch);


// General Video format helper funcs
	BLUEVELVETC_API bool			bfcUtilsIsFormatProgressive(const unsigned int vidFmt);
	BLUEVELVETC_API bool			bfcUtilsIsFormat1001Framerate(const unsigned int vidFmt);
	BLUEVELVETC_API int				bfcUtilsGetFpsForFormat(const unsigned int vidFmt);					// returns closest interger FPS, use Is1001, to determine if div 1001 is needed.

// Get the Bluefish _EVideoMode for the given width, height and fps
	BLUEVELVETC_API int				bfcUtilsGetVideoModeForFrameInfo(const BLUE_UINT32 nWidth, const BLUE_UINT32 nHeight, const BLUE_UINT32 nRate, const BLUE_UINT32 bIs1001, const BLUE_UINT32 bIsProgressive, BLUE_UINT32& nVideoMode);
	BLUEVELVETC_API int				bfcUtilsGetFrameInfoForVideoMode(const BLUE_UINT32 nVideoMode, BLUE_UINT32&  nWidth, BLUE_UINT32& nHeight, BLUE_UINT32& nRate, BLUE_UINT32& bIs1001, BLUE_UINT32& bIsProgressive);
	
	

// HANC / VANC utils

/**
 @brief enumerator used by VANC manipulation function on HD cards to notify whether
 VANC pakcet shoule be inserted/extracted from VANC Y buffers or VANC CbCr buffer.
 This enumerator will only be used on  HD video modes as it is the only with
 2 type of ANC bufers ir Y and CbCr. On SD Modes the ANC data is inserted across
 both Y anc CbCr values.
 
 */
	enum BlueVancPktTypeEnum
	{
		BlueVancPktYComp=0,		/**< ANC pkt should be inserted/extracted from the  Y component buffer*/
		BlueVancPktCbcrComp=1	/**< ANC pkt should be inserted/extracted from the  CbCr component buffer*/
	};

/*!
 @brief Use this function to initialise VANC buffer before inserting any packets into the buffer
 @param CardType type of bluefish  card to which this vanc buffer was transferred to.
 @param nVideoMode video mode under which this vanc buffer will be used.
 @param pixels_per_line width in pixels of the vanc buffer that has to be initialised.
 @param lines_per_frame height of the vanc buffer that has to be initialised.
 @param pVancBuffer vanc buffer which has to be initialised.
 @remarks.
 
 */
	BLUEVELVETC_API BLUE_UINT32 bfcUtilsInitVancBuffer(BLUE_UINT32 CardType,
								   BLUE_UINT32 nVideoMode, 
								   BLUE_UINT32 pixelsPerLine, 
								   BLUE_UINT32 linesPerFrame, 
								   BLUE_UINT32* pVancBuffer);

/*!
 @brief this function can be used to extract ANC packet from HD cards. Currently we can only extract packets in the VANC space.
 @param CardType type of the card from which the vanc buffer was captured.
 @param vanc_pkt_type This parameter denotes whether to search for the VANC packet in Y Space or Cb/Cr Space.
 The values this parameter accepts are defined in the enumerator #BlueVancPktTypeEnum
 @param src_vanc_buffer Vanc buffer which was captured from bluefish card
 @param src_vanc_buffer_size size of the vanc buffer which should be parsed for the specified vanc packet
 @param pixels_per_line specifies how many pixels are there in each line of VANC buffer
 @param vanc_pkt_did specifies the DID of the Vanc packet which should be extracted from the buffer
 @param vanc_pkt_sdid Returns the SDID of the extracted VANC packet
 @param vanc_pkt_data_length returns the size of the extracted VANC packet. The size is specifed as number of UDW words
 that was  contained in the packet
 @param vanc_pkt_data_ptr pointer to UDW of the VANC packets . The 10 bit UDW words are packed in a 16 bit integer. The bottom 10 bit of the
 16 bit word contains the UDW data.
 @param vanc_pkt_line_no line number  where the packet was found .
 
 @remarks.
 
 */
	BLUEVELVETC_API BLUE_INT32 bfcUtilsVancPktExtract( BLUE_UINT32 CardType,
							   BLUE_UINT32 vancPktType,
							   BLUE_UINT32* pSrcVancBuffer,
							   BLUE_UINT32 srcVancBufferSize,
							   BLUE_UINT32 pixelsPerLine,
							   BLUE_UINT32	vancPktDid,
							   BLUE_UINT16* pVancPktSdid,
							   BLUE_UINT16* pVancPktDataLength,
							   BLUE_UINT16* pVancPktData,
							   BLUE_UINT16* pVancPktLineNo);

/**
 @brief use this function to insert ANC packets into the VANC space of the HD cards.
 @param CardType type of the card from which the vanc buffer was captured.
 @param vanc_pkt_type This parameter denotes whether to search for the VANC packet in Y Space or Cb/Cr Space.
 The values this parameter accepts are defined in the enumerator #blue_vanc_pkt_type_enum
 @param vanc_pkt_line_no line in th VANC buffer where the ANC packet should inserted.
 @param vanc_pkt_buffer vanc ANC packet which should be inserted into the VANC buffer.
 @param vanc_pkt_buffer_size size of the ANC packet including the checksum ,ADF , SDID, DID and Data Count
 @param dest_vanc_buffer VANC buffer into which the ANC packet will be inserted into.
 @param pixels_per_line specifies how many pixels are there in each line of VANC buffer
 */
	BLUEVELVETC_API BLUE_INT32  bfcUtilsVancPktInsert(  BLUE_UINT32 CardType,
									BLUE_UINT32 vancPktType,
									BLUE_UINT32 vancPktLineNo,
									BLUE_UINT32* pVancPktBuffer,
									BLUE_UINT32 vancPktBufferSize,
									BLUE_UINT32* pDestVancBuffer,
									BLUE_UINT32 PixelsPerLine);

/** @} */

/**
 @defgroup vanc_decode_encoder_helper ANC encoder/decoder
 @{
 */
	BLUEVELVETC_API BLUE_UINT32 bfcUtilsDecodeEia708bPkt(BLUE_UINT32 CardType,
									 BLUE_UINT16* pVancPktData, 
									 BLUE_UINT16 pktUdwCount, 
									 BLUE_UINT16 eiaPktSubtype, 
									 BLUE_UINT8* pDecodedChStr);
	
#if defined (_WIN32)
} //extern "C"
#endif

#endif // BlueVelvetC_BlueVelvetCUtils_h