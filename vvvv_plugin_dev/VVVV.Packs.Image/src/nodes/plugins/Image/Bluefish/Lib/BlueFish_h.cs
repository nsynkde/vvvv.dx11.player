using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace VVVV.Nodes.Bluefish
{

    public class BlueFish_h
    {
        public enum EVideoMode
        {
            VID_FMT_PAL = 0,
            VID_FMT_NTSC = 1,
            VID_FMT_576I_5000 = 0,	/**< 720  x 576  50       Interlaced */
            VID_FMT_486I_5994 = 1,	/**< 720  x 486  60/1.001 Interlaced */
            VID_FMT_720P_5994,		/**< 1280 x 720  60/1.001 Progressive */
            VID_FMT_720P_6000,		/**< 1280 x 720  60       Progressive */
            VID_FMT_1080PSF_2397,	/**< 1920 x 1080 24/1.001 Segment Frame */
            VID_FMT_1080PSF_2400,	/**< 1920 x 1080 24       Segment Frame */
            VID_FMT_1080P_2397,		/**< 1920 x 1080 24/1.001 Progressive */
            VID_FMT_1080P_2400,		/**< 1920 x 1080 24       Progressive */
            VID_FMT_1080I_5000,		/**< 1920 x 1080 50       Interlaced */
            VID_FMT_1080I_5994,		/**< 1920 x 1080 60/1.001 Interlaced */
            VID_FMT_1080I_6000,		/**< 1920 x 1080 60       Interlaced */
            VID_FMT_1080P_2500,		/**< 1920 x 1080 25       Progressive */
            VID_FMT_1080P_2997,		/**< 1920 x 1080 30/1.001 Progressive */
            VID_FMT_1080P_3000,		/**< 1920 x 1080 30       Progressive */
            VID_FMT_HSDL_1498,		/**< 2048 x 1556 15/1.0   Segment Frame */
            VID_FMT_HSDL_1500,		/**< 2048 x 1556 15			Segment Frame */
            VID_FMT_720P_5000,		/**< 1280 x 720  50			Progressive */
            VID_FMT_720P_2398,		/**< 1280 x 720  24/1.001	Progressive */
            VID_FMT_720P_2400,		/**< 1280 x 720  24	Progressive */
            VID_FMT_2048_1080PSF_2397 = 19,	/**< 2048 x 1080 24/1.001	Segment Frame */
            VID_FMT_2048_1080PSF_2400 = 20,	/**< 2048 x 1080 24	Segment Frame */
            VID_FMT_2048_1080P_2397 = 21,	/**< 2048 x 1080 24/1.001	progressive */
            VID_FMT_2048_1080P_2400 = 22,	/**< 2048 x 1080 24	progressive  */
            VID_FMT_1080PSF_2500 = 23,
            VID_FMT_1080PSF_2997 = 24,
            VID_FMT_1080PSF_3000 = 25,
            VID_FMT_1080P_5000 = 26,
            VID_FMT_1080P_5994 = 27,
            VID_FMT_1080P_6000 = 28,
            VID_FMT_720P_2500 = 29,
            VID_FMT_720P_2997 = 30,
            VID_FMT_720P_3000 = 31,
            VID_FMT_DVB_ASI = 32,
            VID_FMT_2048_1080PSF_2500 = 33,
            VID_FMT_2048_1080PSF_2997 = 34,
            VID_FMT_2048_1080PSF_3000 = 35,
            VID_FMT_2048_1080P_2500 = 36,
            VID_FMT_2048_1080P_2997 = 37,
            VID_FMT_2048_1080P_3000 = 38,
            VID_FMT_2048_1080P_5000 = 39,
            VID_FMT_2048_1080P_5994 = 40,
            VID_FMT_2048_1080P_6000 = 41,
            VID_FMT_1080P_4800 = 42,
            VID_FMT_2048_1080P_4800 = 43,

            VID_FMT_INVALID = 44
        };

        public enum EMemoryFormat
        {
            MEM_FMT_ARGB = 0, /**< ARGB 4:4:4:4 */
            MEM_FMT_BV10 = 1,
            MEM_FMT_BV8 = 2,
            MEM_FMT_YUVS = MEM_FMT_BV8,
            MEM_FMT_V210 = 3,	// Iridium HD (BAG1)
            MEM_FMT_RGBA = 4,
            MEM_FMT_CINEON_LITTLE_ENDIAN = 5,
            MEM_FMT_ARGB_PC = 6,
            MEM_FMT_BGRA = MEM_FMT_ARGB_PC,
            MEM_FMT_CINEON = 7,
            MEM_FMT_2VUY = 8,
            MEM_FMT_BGR = 9,
            MEM_FMT_BGR_16_16_16 = 10,
            MEM_FMT_BGRA_16_16_16_16 = 11,
            MEM_FMT_VUYA_4444 = 12,
            MEM_FMT_V216 = 13,
            MEM_FMT_Y210 = 14,
            MEM_FMT_Y216 = 15,
            MEM_FMT_RGB = 16,
            MEM_FMT_YUV_ALPHA = 17,
            MEM_FMT_INVALID = 18
        };

        public enum EUpdateMethod
        {
            UPD_FMT_FIELD = 0,
            UPD_FMT_FRAME,
            UPD_FMT_FRAME_DISPLAY_FIELD1,
            UPD_FMT_FRAME_DISPLAY_FIELD2,
            UPD_FMT_INVALID,
            /*
            UPD_FMT_FLAG_RETURN_CURRENT_UNIQUEID = 0x80000000,/**< if this flag is used on epoch cards, function would 
													return the unique id of the current frame as the return value.*/
            
        };
    /*    
        public class EUpdateMethod {
	        public static int UPD_FMT_FLAG_RETURN_CURRENT_UNIQUEID = 0x80000000;
        }
      */  
        

        public enum EResoFormat
        {
            RES_FMT_NORMAL = 0,
            RES_FMT_HALF,
            RES_FMT_INVALID
        };


        public enum ECardType
        {
            CRD_BLUEDEEP_LT = 0,		// D64 Lite
            CRD_BLUEDEEP_SD,		// Iridium SD
            CRD_BLUEDEEP_AV,		// Iridium AV
            CRD_BLUEDEEP_IO,		// D64 Full
            CRD_BLUEWILD_AV,		// D64 AV
            CRD_IRIDIUM_HD,			// * Iridium HD
            CRD_BLUEWILD_RT,		// D64 RT
            CRD_BLUEWILD_HD,		// * BadAss G2
            CRD_REDDEVIL,			// Iridium Full
            CRD_BLUEDEEP_HD,		// * BadAss G2 variant, proposed, reserved
            CRD_BLUE_EPOCH_2K = CRD_BLUEDEEP_HD,
            CRD_BLUE_EPOCH_2K_HORIZON = CRD_BLUE_EPOCH_2K,
            CRD_BLUEDEEP_HDS,		// * BadAss G2 variant, proposed, reserved
            CRD_BLUE_ENVY,			// Mini Din 
            CRD_BLUE_PRIDE,			//Mini Din Output 
            CRD_BLUE_GREED,
            CRD_BLUE_INGEST,
            CRD_BLUE_SD_DUALLINK,
            CRD_BLUE_CATALYST,
            CRD_BLUE_SD_DUALLINK_PRO,
            CRD_BLUE_SD_INGEST_PRO,
            CRD_BLUE_SD_DEEPBLUE_LITE_PRO,
            CRD_BLUE_SD_SINGLELINK_PRO,
            CRD_BLUE_SD_IRIDIUM_AV_PRO,
            CRD_BLUE_SD_FIDELITY,
            CRD_BLUE_SD_FOCUS,
            CRD_BLUE_SD_PRIME,
            CRD_BLUE_EPOCH_2K_CORE,
            CRD_BLUE_EPOCH_2K_ULTRA,
            CRD_BLUE_EPOCH_HORIZON,
            CRD_BLUE_EPOCH_CORE,
            CRD_BLUE_EPOCH_ULTRA,
            CRD_BLUE_CREATE_HD,
            CRD_BLUE_CREATE_2K,
            CRD_BLUE_CREATE_2K_ULTRA,
            CRD_BLUE_CREATE_3D = CRD_BLUE_CREATE_2K,
            CRD_BLUE_CREATE_3D_ULTRA = CRD_BLUE_CREATE_2K_ULTRA,
            CRD_BLUE_SUPER_NOVA,
            CRD_BLUE_SUPER_NOVA_S_PLUS,
            CRD_INVALID
        };


        public enum EHDCardSubType
        {
            CRD_HD_FURY = 1,
            CRD_HD_VENGENCE = 2,
            CRD_HD_IRIDIUM_XP = 3,
            CRD_HD_IRIDIUM = 4,
            CRD_HD_LUST = 5,
            CRD_HD_INVALID
        };

        public enum EEpochFirmwareProductID
        {
            ORAC_FILMPOST_FIRMWARE_PRODUCTID = (0x1),				//Epoch/Create, standard firmware
            ORAC_BROADCAST_FIRMWARE_PRODUCTID = (0x2),			//Epoch
            ORAC_ASI_FIRMWARE_PRODUCTID = (0x3),					//Epoch
            ORAC_4SDIINPUT_FIRMWARE_PRODUCTID = (0x4),			//SuperNova
            ORAC_4SDIOUTPUT_FIRMWARE_PRODUCTID = (0x5),			//SuperNova
            ORAC_2SDIINPUT_2SDIOUTPUT_FIRMWARE_PRODUCTID = (0x6),	//SuperNova
            ORAC_1SDIINPUT_3SDIOUTPUT_FIRMWARE_PRODUCTID = (0x8),	//SuperNova
            ORAC_INPUT_1SDI_1CHANNEL_OUTPUT_4SDI_3CHANNEL_FIRMWARE_PRODUCTID = (0x9),	//SuperNova
            ORAC_INPUT_2SDI_2CHANNEL_OUTPUT_3SDI_2CHANNEL_FIRMWARE_PRODUCTID = (0xA),	//SuperNova
            ORAC_INPUT_3SDI_3CHANNEL_OUTPUT_1SDI_1CHANNEL_FIRMWARE_PRODUCTID = (0xB),	//SuperNova
            ORAC_BNC_ASI_FIRMWARE_PRODUCTID = (0xC),	//SuperNova
        };


        //public enum BlueAudioChannelDesc
        //{
        //    MONO_FLAG = (long)0xC0000000,
        //    MONO_CHANNEL_1 = 0x00000001,
        //    MONO_CHANNEL_2 = 0x00000002,
        //    MONO_CHANNEL_3 = 0x00000004,
        //    MONO_CHANNEL_4 = 0x00000008,
        //    MONO_CHANNEL_5 = 0x00000010,
        //    MONO_CHANNEL_6 = 0x00000020,
        //    MONO_CHANNEL_7 = 0x00000040,
        //    MONO_CHANNEL_8 = 0x00000080,
        //    MONO_CHANNEL_9 = 0x00000100,// to be used by analog audio output channels 
        //    MONO_CHANNEL_10 = 0x00000200,// to be used by analog audio output channels 
        //    MONO_CHANNEL_11 = 0x00000400,//actual channel 9
        //    MONO_CHANNEL_12 = 0x00000800,//actual channel 10
        //    MONO_CHANNEL_13 = 0x00001000,//actual channel 11
        //    MONO_CHANNEL_14 = 0x00002000,//actual channel 12
        //    MONO_CHANNEL_15 = 0x00004000,//actual channel 13
        //    MONO_CHANNEL_16 = 0x00008000,//actual channel 14
        //    MONO_CHANNEL_17 = 0x00010000,//actual channel 15
        //    MONO_CHANNEL_18 = 0x00020000 //actual channel 16
        //};

        public enum EAudioFlags
        {
	        AUDIO_CHANNEL_LOOPING_OFF		= 0x00000000, /**< deprecated not used any more */
	        AUDIO_CHANNEL_LOOPING			= 0x00000001,/**< deprecated not used any more */
	        AUDIO_CHANNEL_LITTLEENDIAN		= 0x00000000, /**< if the audio data is little endian this flag must be set*/
	        AUDIO_CHANNEL_BIGENDIAN			= 0x00000002,/**< if the audio data is big  endian this flag must be set*/
	        AUDIO_CHANNEL_OFFSET_IN_BYTES	= 0x00000004,/**< deprecated not used any more */
	        AUDIO_CHANNEL_16BIT				= 0x00000008, /**< if the audio channel bit depth is 16 bits this flag must be set*/
	        AUDIO_CHANNEL_BLIP_PENDING		= 0x00000010,/**< deprecated not used any more */
	        AUDIO_CHANNEL_BLIP_COMPLETE		= 0x00000020,/**< deprecated not used any more */
	        AUDIO_CHANNEL_SELECT_CHANNEL	= 0x00000040,/**< deprecated not used any more */
	        AUDIO_CHANNEL_24BIT				= 0x00000080/**< if the audio channel bit depth is 24 bits this flag must be set*/
        };

        public enum Blue_Audio_Connector_Type {
	        BLUE_AUDIO_AES=0, /**< Used to select All 8 channels of Digital Audio using AES/AES3id  connector*/
	        BLUE_AUDIO_ANALOG=1,/**< Used to select Analog audio*/
	        BLUE_AUDIO_SDIA=2, /**< Used to select Emb audio from SDI A */
	        BLUE_AUDIO_EMBEDDED=BLUE_AUDIO_SDIA,
	        BLUE_AUDIO_SDIB=3, /**< Used to select Emb audio from SDI B */
	        BLUE_AUDIO_AES_PAIR0=4, /**< Used to select stereo pair 0 as audio input source. This is only supported on SD Greed Derivative cards.*/
	        BLUE_AUDIO_AES_PAIR1=5,/**< Used to select stereo pair 1 as audio input source. This is only supported on SD Greed Derivative cards.*/
	        BLUE_AUDIO_AES_PAIR2=6,/**< Used to select stereo pair 2 as audio input source. This is only supported on SD Greed Derivative cards.*/
	        BLUE_AUDIO_AES_PAIR3=7,/**< Used to select stereo pair 3 as audio input source. This is only supported on SD Greed Derivative cards.*/
	        BLUE_AUDIO_SDIC=8, /**< Used to select Emb audio from SDI C */
	        BLUE_AUDIO_SDID=9, /**< Used to select Emb audio from SDI D */
	        BLUE_AUDIO_INVALID=10
        };

        public enum EAudioRate
        {
	        AUDIO_SAMPLE_RATE_48K=48000,
	        AUDIO_SAMPLE_RATE_96K=96000,
	        AUDIO_SAMPLE_RATE_UNKNOWN=-1
        };

        public enum EConnectorSignalColorSpace
        {
            RGB_ON_CONNECTOR = 0x00400000, /**< Use this enumerator if the colorspace of video data on the SDI cable is RGB <br/>
										When using dual link capture/playback , user can choose the 
										color space of the data. <br>
										In single link SDI the color space of the signal is always YUB*/
            YUV_ON_CONNECTOR = 0 /**<Use this enumerator if color space of video data on the SDI cable  is RGB.*/
        };

        public enum EDualLinkSignalFormatType
        {
            Signal_FormatType_4224 = 0, /**< sets the card to work in  4:2:2:4 mode*/
            Signal_FormatType_4444 = 1,/**< sets the card to work in  4:4:4 10 bit dual link mode*/
            Signal_FormatType_444_10BitSDI = Signal_FormatType_4444,/**< sets the card to work in  10 bit 4:4:4 dual link mode*/
            Signal_FormatType_444_12BitSDI = 0x4,/**< sets the card to work in  4:4:4 12 bit dual link mode*/
            Signal_FormatType_Independent_422 = 0x2,
            Signal_FormatType_Key_Key = 0x8000/**< not used currently on epoch cards */

        };

        public enum ECardOperatingMode
        {
            CardOperatingMode_SingleLink = 0x0,
            CardOperatingMode_Independent_422 = CardOperatingMode_SingleLink,
            CardOperatingMode_DualLink = 0x1,
            CardOperatingMode_StereoScopic_422 = 0x3,
            CardOperatingMode_Dependent_422 = CardOperatingMode_StereoScopic_422,/**< not used currently on epoch cards */
        };

        public enum EPreDefinedColorSpaceMatrix
        {
            UNITY_MATRIX = 0,
            MATRIX_709_CGR = 1,
            MATRIX_RGB_TO_YUV_709_CGR = MATRIX_709_CGR,
            MATRIX_709 = 2,
            MATRIX_RGB_TO_YUV_709 = MATRIX_709,
            RGB_FULL_RGB_SMPTE = 3,
            MATRIX_601_CGR = 4,
            MATRIX_RGB_TO_YUV_601_CGR = MATRIX_601_CGR,
            MATRIX_601 = 5,
            MATRIX_RGB_TO_YUV_601 = MATRIX_601,
            MATRIX_SMPTE_274_CGR = 6,
            MATRIX_SMPTE_274 = 7,
            MATRIX_VUYA = 8,
            UNITY_MATRIX_INPUT = 9,
            MATRIX_YUV_TO_RGB_709_CGR = 10,
            MATRIX_YUV_TO_RGB_709 = 11,
            RGB_SMPTE_RGB_FULL = 12,
            MATRIX_YUV_TO_RGB_601_CGR = 13,
            MATRIX_YUV_TO_RGB_601 = 14,
            MATRIX_USER_DEFINED = 15,
        };

        public enum BlueVideoFifoStatus
        {
	        BLUE_FIFO_CLOSED=0, /**< Fifo has not been initialized*/
	        BLUE_FIFO_STARTING=1,/**< Fifo is starting */
	        BLUE_FIFO_RUNNING=2,/**< Fifo is running */
	        BLUE_FIFO_STOPPING=3,/**< Fifo is in the process of stopping */
	        BLUE_FIFO_PASSIVE=5,/**< Fifo is currently stopped or not active*/
        };

        public enum ERGBDataRange
        {
            CGR_RANGE = 0, /**<	In this mode RGB data expected by the user (capture) or provided by the user(playback) is 
						in the range of 0-255(8 bit) or 0-1023(10 bit0).<br/>
						driver uses this information to choose the appropriate YUV conversion matrices.*/
            SMPTE_RANGE = 1  /**<	In this mode RGB data expected by the user (capture) or provided by the user(playback) is 
						in the range of 16-235(8 bit) or 64-940(10 bit0).<br/>
						driver uses this information to choose the appropriate YUV conversion matrices.*/
        };

        public enum HD_XCONNECTOR_MODE
        {
            SD_SDI = 1,
            HD_SDI = 2
        };

        public enum EImageOrientation
        {
            ImageOrientation_Normal = 0,	/**< in this configuration , frame is top to bottom and left to right */
            ImageOrientation_VerticalFlip = 1, /**< in this configuration frame is bottom to top and left to right*/
            ImageOrientation_Invalid = 2,
        };

        public enum EBlueGenlockSource
        {
            BlueGenlockBNC = 0, /**< Genlock is used as reference signal source */
            BlueSDIBNC = 0x10000, /**< SDI input B  is used as reference signal source */
            BlueSDI_B_BNC = BlueSDIBNC,
            BlueSDI_A_BNC = 0x20000,/**< SDI input A  is used as reference signal source */
            BlueAnalog_BNC = 0x40000, /**< Analog input is used as reference signal source */
            BlueSoftware = 0x80000,
        };

        public enum EBlueVideoChannel
        {
            BLUE_VIDEOCHANNEL_A = 0,
            BLUE_VIDEO_OUTPUT_CHANNEL_A = BLUE_VIDEOCHANNEL_A,

            BLUE_VIDEOCHANNEL_B = 1,
            BLUE_VIDEO_OUTPUT_CHANNEL_B = BLUE_VIDEOCHANNEL_B,

            BLUE_VIDEOCHANNEL_C = 2,
            BLUE_VIDEO_INPUT_CHANNEL_A = BLUE_VIDEOCHANNEL_C,

            BLUE_VIDEOCHANNEL_D = 3,
            BLUE_VIDEO_INPUT_CHANNEL_B = BLUE_VIDEOCHANNEL_D,

            BLUE_VIDEOCHANNEL_E = 4,
            BLUE_VIDEO_INPUT_CHANNEL_C = BLUE_VIDEOCHANNEL_E,

            BLUE_VIDEOCHANNEL_F = 5,
            BLUE_VIDEO_INPUT_CHANNEL_D = BLUE_VIDEOCHANNEL_F,

            BLUE_VIDEOCHANNEL_G = 6,
            BLUE_VIDEO_OUTPUT_CHANNEL_C = BLUE_VIDEOCHANNEL_G,

            BLUE_VIDEOCHANNEL_H = 7,
            BLUE_VIDEO_OUTPUT_CHANNEL_D = BLUE_VIDEOCHANNEL_H,

            BLUE_OUTPUT_MEM_MODULE_A = BLUE_VIDEO_OUTPUT_CHANNEL_A,
            BLUE_OUTPUT_MEM_MODULE_B = BLUE_VIDEO_OUTPUT_CHANNEL_B,
            BLUE_INPUT_MEM_MODULE_A = BLUE_VIDEO_INPUT_CHANNEL_A,
            BLUE_INPUT_MEM_MODULE_B = BLUE_VIDEO_INPUT_CHANNEL_B,
            //BLUE_JETSTREAM_SCALER_MODULE_0=0x10,
            //BLUE_JETSTREAM_SCALER_MODULE_1=0x11,
            //BLUE_JETSTREAM_SCALER_MODULE_2=0x12,
            //BLUE_JETSTREAM_SCALER_MODULE_3=0x13,

            BLUE_VIDEOCHANNEL_INVALID = 30
        };

        public enum EBlueVideoRouting
        {
            BLUE_VIDEO_LINK_INVALID = 0,
            BLUE_SDI_A_LINK1 = 4,
            BLUE_SDI_A_LINK2 = 5,
            BLUE_SDI_B_LINK1 = 6,
            BLUE_SDI_B_LINK2 = 7,
            BLUE_ANALOG_LINK1 = 8,
            BLUE_ANALOG_LINK2 = 9,
            BLUE_SDI_A_SINGLE_LINK = BLUE_SDI_A_LINK1,
            BLUE_SDI_B_SINGLE_LINK = BLUE_SDI_B_LINK1,
            BLUE_ANALOG_SINGLE_LINK = BLUE_ANALOG_LINK1

        };

        public enum BlueVideoFifo_Attributes
        {
	        BLUE_FIFO_NULL_ATTRIBUTE=0x0,
	        BLUE_FIFO_ECHOPORT_ENABLED=0x1,
	        BLUE_FIFO_STEPMODE = 0x2,
	        BLUE_FIFO_LOOPMODE = 0x4
        };

        public enum BlueAudioOutputDest
        {
            Blue_AnalogAudio_Output = 0x0,
            /*
            Blue_AES_Output = 0x80000000,
            Blue_Emb_Output = 0x40000000,
             */
        };

        public enum BlueAudioInputSource
        {
            Blue_AES = 0x10,
            Blue_AnalogAudio = 0x20,
            Blue_SDIA_Embed = 0x40,
            Blue_SDIB_Embed = 0x80,
        };

        public enum EBlueConnectorIdentifier
        {
            BLUE_CONNECTOR_INVALID = -1,

            // BNC connectors in order from top to bottom of shield
            BLUE_CONNECTOR_BNC_A = 0,    // BNC closest to top of shield
            BLUE_CONNECTOR_BNC_B,
            BLUE_CONNECTOR_BNC_C,
            BLUE_CONNECTOR_BNC_D,
            BLUE_CONNECTOR_BNC_E,
            BLUE_CONNECTOR_BNC_F,
            BLUE_CONNECTOR_GENLOCK,

            BLUE_CONNECTOR_ANALOG_VIDEO_1 = 100,
            BLUE_CONNECTOR_ANALOG_VIDEO_2,
            BLUE_CONNECTOR_ANALOG_VIDEO_3,
            BLUE_CONNECTOR_ANALOG_VIDEO_4,
            BLUE_CONNECTOR_ANALOG_VIDEO_5,
            BLUE_CONNECTOR_ANALOG_VIDEO_6,

            BLUE_CONNECTOR_DVID_1 = 200,
            BLUE_CONNECTOR_SDI_OUTPUT_A = BLUE_CONNECTOR_DVID_1,
            BLUE_CONNECTOR_DVID_2,
            BLUE_CONNECTOR_SDI_OUTPUT_B = BLUE_CONNECTOR_DVID_2,
            BLUE_CONNECTOR_DVID_3,
            BLUE_CONNECTOR_SDI_INPUT_A = BLUE_CONNECTOR_DVID_3,
            BLUE_CONNECTOR_DVID_4,
            BLUE_CONNECTOR_SDI_INPUT_B = BLUE_CONNECTOR_DVID_4,
            BLUE_CONNECTOR_DVID_5,
            BLUE_CONNECTOR_SDI_OUTPUT_C,
            BLUE_CONNECTOR_SDI_OUTPUT_D,

            BLUE_CONNECTOR_AES = 300,
            BLUE_CONNECTOR_ANALOG_AUDIO_1,
            BLUE_CONNECTOR_ANALOG_AUDIO_2,

            BLUE_CONNECTOR_DVID_6,
            BLUE_CONNECTOR_SDI_INPUT_C = BLUE_CONNECTOR_DVID_6,
            BLUE_CONNECTOR_DVID_7,
            BLUE_CONNECTOR_SDI_INPUT_D = BLUE_CONNECTOR_DVID_7,

            //BLUE_CONNECTOR_RESOURCE_BLOCK=0x400,
            //BLUE_CONNECTOR_JETSTREAM_SCALER_0=(BLUE_CONNECTOR_RESOURCE_BLOCK|BLUE_JETSTREAM_SCALER_MODULE_0),
            //BLUE_CONNECTOR_JETSTREAM_SCALER_1=(BLUE_CONNECTOR_RESOURCE_BLOCK|BLUE_JETSTREAM_SCALER_MODULE_1),
            //BLUE_CONNECTOR_JETSTREAM_SCALER_2=(BLUE_CONNECTOR_RESOURCE_BLOCK|BLUE_JETSTREAM_SCALER_MODULE_2),
            //BLUE_CONNECTOR_JETSTREAM_SCALER_3=(BLUE_CONNECTOR_RESOURCE_BLOCK|BLUE_JETSTREAM_SCALER_MODULE_3),

            //BLUE_CONNECTOR_OUTPUT_MEM_MODULE_A=(BLUE_CONNECTOR_RESOURCE_BLOCK|BLUE_OUTPUT_MEM_MODULE_A),
            //BLUE_CONNECTOR_OUTPUT_MEM_MODULE_B=(BLUE_CONNECTOR_RESOURCE_BLOCK|BLUE_OUTPUT_MEM_MODULE_B),
            //BLUE_CONNECTOR_INPUT_MEM_MODULE_A=(BLUE_CONNECTOR_RESOURCE_BLOCK|BLUE_INPUT_MEM_MODULE_A),
            //BLUE_CONNECTOR_INPUT_MEM_MODULE_B=(BLUE_CONNECTOR_RESOURCE_BLOCK|BLUE_INPUT_MEM_MODULE_B),
            //
        };

        public enum EBlueConnectorSignalDirection
        {
            BLUE_CONNECTOR_SIGNAL_INVALID = -1,
            BLUE_CONNECTOR_SIGNAL_INPUT = 0,
            BLUE_CONNECTOR_SIGNAL_OUTPUT = 1,
        };

        public enum EBlueConnectorProperty
        {
            BLUE_INVALID_CONNECTOR_PROPERTY = -1,

            //signal property
            BLUE_CONNECTOR_PROP_INPUT_SIGNAL = 0,
            BLUE_CONNECTOR_PROP_OUTPUT_SIGNAL = 1,

            // Video output
            BLUE_CONNECTOR_PROP_SDI = 0,
            BLUE_CONNECTOR_PROP_YUV_Y,
            BLUE_CONNECTOR_PROP_YUV_U,
            BLUE_CONNECTOR_PROP_YUV_V,
            BLUE_CONNECTOR_PROP_RGB_R,
            BLUE_CONNECTOR_PROP_RGB_G,
            BLUE_CONNECTOR_PROP_RGB_B,
            BLUE_CONNECTOR_PROP_CVBS,
            BLUE_CONNECTOR_PROP_SVIDEO_Y,
            BLUE_CONNECTOR_PROP_SVIDEO_C,

            // Audio output
            BLUE_CONNECTOR_PROP_AUDIO_AES = 0x2000,
            BLUE_CONNECTOR_PROP_AUDIO_EMBEDDED,
            BLUE_CONNECTOR_PROP_AUDIO_ANALOG,


            BLUE_CONNECTOR_PROP_SINGLE_LINK = 0x3000,
            BLUE_CONNECTOR_PROP_DUALLINK_LINK_1,
            BLUE_CONNECTOR_PROP_DUALLINK_LINK_2,
            BLUE_CONNECTOR_PROP_DUALLINK_LINK,

            BLUE_CONNECTOR_PROP_STEREO_MODE_SIDE_BY_SIDE,
            BLUE_CONNECTOR_PROP_STEREO_MODE_TOP_DOWN,
            BLUE_CONNECTOR_PROP_STEREO_MODE_LINE_BY_LINE,

        };

        public enum EBlueCardProperty
        {
            VIDEO_DUAL_LINK_OUTPUT = 0,/**<  Use this property to enable/diable cards dual link output property*/
            VIDEO_DUAL_LINK_INPUT = 1,/**<  Use this property to enable/diable cards dual link input property*/
            VIDEO_DUAL_LINK_OUTPUT_SIGNAL_FORMAT_TYPE = 2, /**<Use this property to select signal format type that should be used 
							when dual link output is enabled. Possible values this property can
							accept is defined in the enumerator EDualLinkSignalFormatType
												*/
            VIDEO_DUAL_LINK_INPUT_SIGNAL_FORMAT_TYPE = 3,/**<	Use this property to select signal format type that should be used 
						when dual link input is enabled. Possible values this property can
						accept is defined in the enumerator EDualLinkSignalFormatType
						*/
            VIDEO_OUTPUT_SIGNAL_COLOR_SPACE = 4,/**<	Use this property to select color space of the signal when dual link output is set to 
					use 4:4:4/4:4:4:4 signal format type. Possible values this property can
					accept is defined in the enumerator EConnectorSignalColorSpace
					*/
            VIDEO_INPUT_SIGNAL_COLOR_SPACE = 5,/**<	Use this property to select color space of the signal when dual link input is set to 
					use 4:4:4/4:4:4:4 signal format type. Possible values this property can
					accept is defined in the enumerator EConnectorSignalColorSpace
					*/
            VIDEO_MEMORY_FORMAT = 6, /**<Use this property to ser the pixel format that should be used by 
			video output channels.  Possible values this property can
			accept is defined in the enumerator EMemoryFormat
			*/
            VIDEO_MODE = 7,/**<Use this property to set the video mode that should be used by 
			video output channels.  Possible values this property can
			accept is defined in the enumerator EVideoMode
			*/
            //
            VIDEO_UPDATE_TYPE = 8,/**<Use this property to set the framestore update type that should be used by 
			video output channels.  Card can update video framestore at field/frame rate.
			Possible values this property can accept is defined in the enumerator EUpdateMethod
			*/
            VIDEO_ENGINE = 9,
            VIDEO_IMAGE_ORIENTATION = 10,/**<	Use this property to set the image orientation of the video output framestore.
				This property must be set before frame is transferred to on card memory using 
				DMA transfer functions(system_buffer_write_async). It is recommended to use 
				vertical flipped image orientation only on RGB pixel formats.
				Possible values this property can accept is defined in the enumerator EImageOrientation
				*/
            VIDEO_USER_DEFINED_COLOR_MATRIX = 11,
            VIDEO_PREDEFINED_COLOR_MATRIX = 12,//EPreDefinedColorSpaceMatrix
            VIDEO_RGB_DATA_RANGE = 13,		/**<	Use this property to set the data range of RGB pixel format, user can specify 
											whether the RGB data is in either SMPTE or CGR range. Based on this information 
											driver is decide which color matrix should be used.
											Possible values this property can accept is defined in the enumerator ERGBDataRange
											For SD cards this property will set the input and the output to the specified value.
											For Epoch/Create/SuperNova cards this property will only set the output to the specified value.
											For setting the input on Epoch/Create/SuperNova cards see EPOCH_VIDEO_INPUT_RGB_DATA_RANGE*/
            VIDEO_KEY_OVER_BLACK = 14,/**< this property is deprecated and no longer supported on epoch/create range of cards.*/
            VIDEO_KEY_OVER_INPUT_SIGNAL = 15,
            VIDEO_SET_DOWN_CONVERTER_VIDEO_MODE = 16,/**< this property is deprecated and no longer supported on epoch/create range of cards.
						EHD_XCONNECTOR_MODE
						*/
            VIDEO_LETTER_BOX = 17,
            VIDEO_PILLOR_BOX_LEFT = 18,
            VIDEO_PILLOR_BOX_RIGHT = 19,
            VIDEO_PILLOR_BOX_TOP = 20,
            VIDEO_PILLOR_BOX_BOTTOM = 21,
            VIDEO_SAFE_PICTURE = 22,
            VIDEO_SAFE_TITLE = 23,
            VIDEO_INPUT_SIGNAL_VIDEO_MODE = 24,	/**< Use this property to retreive the video input signal information on the 
											default video input channel used by that SDK object.
											When calling SetCardProperty with a valid video mode on this property, the SDK
											will will use this video mode "Hint" if the card buffers are set up despite there being a valid 
											input signal; the card buffers will be set up when calling one of these card properties:
												VIDEO_INPUT_MEMORY_FORMAT
												VIDEO_INPUT_UPDATE_TYPE
												VIDEO_INPUT_ENGINE
											Note: QueryCardProperty(VIDEO_INPUT_SIGNAL_VIDEO_MODE) will still return the actual video input signal
										*/
            VIDEO_COLOR_MATRIX_MODE = 25,
            VIDEO_OUTPUT_MAIN_LUT = 26,/**< this property is deprecated and no longer supported on epoch/create range of cards.*/
            VIDEO_OUTPUT_AUX_LUT = 27,/**< this property is deprecated and no longer supported on epoch/create range of cards.*/
            VIDEO_LTC = 28,		/**< this property is deprecated and no longer supported on epoch/create range of cards. To retreive/ outputting 
				LTC information you can use the HANC decoding and encoding functions.*/
            VIDEO_GPIO = 29,
            VIDEO_PLAYBACK_FIFO_STATUS = 30, /**< This property can be used to retreive how many frames are bufferd in the video playback fifo.*/
            RS422_RX_BUFFER_LENGTH = 31,
            RS422_RX_BUFFER_FLUSH = 32,
            VIDEO_INPUT_UPDATE_TYPE = 33,/**<	Use this property to set the framestore update type that should be used by 
				video input  channels.  Card can update video framestore at field/frame rate.
				Possible values this property can accept is defined in the enumerator EUpdateMethod
									*/
            VIDEO_INPUT_MEMORY_FORMAT = 34,/**<Use this property to set the pixel format that should be used by 
					video input channels when it is capturing a frame from video input source.  
					Possible values this property can accept is defined in the enumerator EMemoryFormat
									*/
            VIDEO_GENLOCK_SIGNAL = 35,/**< Use this property to retrieve video signal of the reference source that is used by the card.
				This can also be used to select the reference signal source that should be used. 	
				*/

            AUDIO_OUTPUT_PROP = 36,	/**< this can be used to route PCM audio data onto respective audio output connectors. */
            AUDIO_CHANNEL_ROUTING = AUDIO_OUTPUT_PROP,
            AUDIO_INPUT_PROP = 37,/**< Use this property to select audio input source that should be used when doing 
			an audio capture.
			Possible values this property can accept is defined in the enumerator Blue_Audio_Connector_Type.
			*/
            VIDEO_ENABLE_LETTERBOX = 38,
            VIDEO_DUALLINK_OUTPUT_INVERT_KEY_COLOR = 39,/**< this property is deprecated and no longer supported on epoch/create range of cards.*/
            VIDEO_DUALLINK_OUTPUT_DEFAULT_KEY_COLOR = 40,/**< this property is deprecated and no longer supported on epoch/create range of cards.*/
            VIDEO_BLACKGENERATOR = 41, /**< Use this property to control the black generator on the video output channel.
							 */
            VIDEO_INPUTFRAMESTORE_IMAGE_ORIENTATION = 42,
            VIDEO_INPUT_SOURCE_SELECTION = 43, /**<	The video input source that should be used by the SDK default video input channel 
					can be configured using this property.	
					Possible values this property can accept is defined in the enumerator EBlueConnectorIdentifier.
					*/
            DEFAULT_VIDEO_OUTPUT_CHANNEL = 44,
            DEFAULT_VIDEO_INPUT_CHANNEL = 45,
            VIDEO_REFERENCE_SIGNAL_TIMING = 46,
            EMBEDEDDED_AUDIO_OUTPUT = 47, /**< the embedded audio output property can be configured using this property.
					Possible values this property can accept is defined in the enumerator EBlueEmbAudioOutput.
					*/
            EMBEDDED_AUDIO_OUTPUT = EMBEDEDDED_AUDIO_OUTPUT,
            VIDEO_PLAYBACK_FIFO_FREE_STATUS = 48, /**< this will return the number of free buffer in the fifo. 
					If the video engine  is framestore this will give you the number of buffers that the framestore mode 
					can you use with that video output channel.*/
            VIDEO_IMAGE_WIDTH = 49,		/**< selective output DMA: see application note AN008_SelectiveDMA.pdf for more details */
            VIDEO_IMAGE_HEIGHT = 50,		/**< selective output DMA: see application note AN008_SelectiveDMA.pdf for more details */
            VIDEO_SCALER_MODE = 51,
            AVAIL_AUDIO_INPUT_SAMPLE_COUNT = 52,
            VIDEO_PLAYBACK_FIFO_ENGINE_STATUS = 53,	/**< this will return the playback fifo status. The values returned by this property 
						are defined in the enumerator BlueVideoFifoStatus.
						*/
            VIDEO_CAPTURE_FIFO_ENGINE_STATUS = 54,	/**< this will return the capture  fifo status. 
						The values returned by this property are defined in the enumerator BlueVideoFifoStatus.
						*/
            VIDEO_2K_1556_PANSCAN = 55,/**< this property is deprecated and no longer supported on epoch/create range of cards.*/
            VIDEO_OUTPUT_ENGINE = 56,	/**< Use this property to set the video engine of the video output channels.
				Possible values this property can accept is defined in the enumerator EEngineMode 
				*/
            VIDEO_INPUT_ENGINE = 57, /**< Use this property to set the video engine of the video input channels.
				Possible values this property can accept is defined in the enumerator EEngineMode 
				*/
            BYPASS_RELAY_A_ENABLE = 58, /**< use this property to control the bypass relay on SDI A output.*/
            BYPASS_RELAY_B_ENABLE = 59, /**< use this property to control the bypass relay on SDI B output.*/
            VIDEO_PREMULTIPLIER = 60,
            VIDEO_PLAYBACK_START_TRIGGER_POINT = 61, /**< Using this property you can instruct the driver to start the 
						video playback fifo on a particular video output field count.
						Normally video playback fifo is started on the next video interrupt after 
						the video_playback_start call.*/
            GENLOCK_TIMING = 62,
            VIDEO_IMAGE_PITCH = 63,	/**< selective output DMA: see application note AN008_SelectiveDMA.pdf for more details */
            VIDEO_IMAGE_OFFSET = 64,	/**< selective output DMA: see application note AN008_SelectiveDMA.pdf for more details */
            VIDEO_INPUT_IMAGE_WIDTH = 65,		/**< selective input DMA: see application note AN008_SelectiveDMA.pdf for more details */
            VIDEO_INPUT_IMAGE_HEIGHT = 66,	/**< selective input DMA: see application note AN008_SelectiveDMA.pdf for more details */
            VIDEO_INPUT_IMAGE_PITCH = 67,		/**< selective input DMA: see application note AN008_SelectiveDMA.pdf for more details */
            VIDEO_INPUT_IMAGE_OFFSET = 68,	/**< selective input DMA: see application note AN008_SelectiveDMA.pdf for more details */
            TIMECODE_RP188 = 69,	/**< this property is deprecated and no longer supported on epoch/create range of cards.*/
            BOARD_TEMPERATURE = 70,/**<This property can be used to retreive the Board temperature, core temperature and 
				RPM of the Fan on epoch/create range of cards.<br/>
				Use the macro's EPOCH_CORE_TEMP ,EPOCH_BOARD_TEMP and EPOCH_FAN_SPEED
				to retireive the respective values from the property.<br/>							
						*/
            MR2_ROUTING = 71,	/**< Use this property to control the MR2 functionlity on epoch range of cards.
			Use the following macro with this property.<br/>
			1) EPOCH_SET_ROUTING --> for setting the source, destination and link type of the routing connection,<br/>
			2) EPOCH_ROUTING_GET_SRC_DATA --> for getting the routing source.<br/>
							The possible source and destination elements supported by the routing matrix are defined in the 
							enumerator EEpochRoutingElements.<br/>
			*/
            SAVEAS_POWERUP_SETTINGS = 72,
            VIDEO_CAPTURE_AVAIL_BUFFER_COUNT = 73, /**< This property will return the number of captured frame avail in the fifo at present.
						If the video engine  is framestore this will give you the number of buffers that the framestore mode 
						can you use with that video input channel */
            EPOCH_APP_WATCHDOG_TIMER = 74,/**< Use this property to control the application  watchdog timer functionality. 
					Possible values this property can accept is defined in the enumerator enum_blue_app_watchdog_timer_prop.
				*/
            EPOCH_RESET_VIDEO_INPUT_FIELDCOUNT = 75, /**< Use this property to reset the field count on both the 
						video channels of the card. You can pass the value that 
						should be used as starting fieldcount after the reset.
						This property can be used to keep track  sync between left and right signal 
						when you are capturing in stereoscopic mode.
					*/
            EPOCH_RS422_PORT_FLAGS = 76,/**<	Use this property to set the master/slave property of the RS422 ports.
				Possible values this property can accept is defined in the enumerator enum_blue_rs422_port_flags.
				*/
            EPOCH_DVB_ASI_INPUT_TIMEOUT = 77,	/**< Current DVB ASI input firmware does not support this property in hardware,
					this is a future addition.
					Use this property to set the timeout  of the DVB ASI input stream. 
					timeout is specified in  milliseconds.If hardware did not get the required no of 		
					packets( specified using EPOCH_DVB_ASI_INPUT_LATENCY_PACKET_COUNT)
					within the period specified in the timeout, hardware would generate a video input interrupt
					and it would be safe to read the dvb asi packet from the card.
					*/
            EPOCH_DVB_ASI_INPUT_PACKING_FORMAT = 78, /**< Use this property to specify the packing method that should be used 
						when capturing DVB ASI packets.
						The possible packing methods are defined in the enumerator enum_blue_dvb_asi_packing_format.*/
            EPOCH_DVB_ASI_INPUT_LATENCY_PACKET_COUNT = 79, /**< Use this property to set  how many asi packets should be captured by the card , before it 
							notifies the driver of available data using video input interrupt.<br/>
							*/
            VIDEO_PLAYBACK_FIFO_CURRENT_FRAME_UNIQUEID = 80, /**< This property can be used to query the current unique id of 
							the frame that is being displayed currently by the video output channel. This 
							property is only usefull in the context of video fifo.<br/>
							You get a uniqueid when you present a frame using video_playback_present function.
							Alternative ways to get this information are <br/>
							1) using blue_wait_video_sync_async , the member current_display_frame_uniqueid contains the same information<br/>
							2) using wait_video_output_sync function on epoch cards, if 
							the flag UPD_FMT_FLAG_RETURN_CURRENT_UNIQUEID is appended with 
							either UPD_FMT_FRAME or UPD_FMT_FIELD , the return value of 
							the function wait_video_output_sync woukd contain the current display
							frames uniqueid.<br/>*/

            EPOCH_DVB_ASI_INPUT_GET_PACKET_SIZE = 81,/**< use this property to get the size of each asi transport stream packet
							(whether it is 188 or 204.*/
            EPOCH_DVB_ASI_INPUT_PACKET_COUNT = 82,/**< this property would give you the number of packets captured during the last 
						interrupt time frame. For ASI interrupt is generated if 
						hardware captured the requested number of packets or it hit the 
						timeout value
						*/
            EPOCH_DVB_ASI_INPUT_LIVE_PACKET_COUNT = 83,/**< this property would give you the number of packets that
						is being captured during the current interrupt time frame. 
						For ASI interrupt is generated when has hardware captured the 
						requested number of packets specified using  
						EPOCH_DVB_ASI_INPUT_LATENCY_PACKET_COUNT property.
						*/
            EPOCH_DVB_ASI_INPUT_AVAIL_PACKETS_IN_FIFO = 84,/**< This property would return the number of ASI packets 
							that has been captured into card memory , that
							can be retreived.
							This property is only valid when the video input 
							channel is being used in FIFO modes.
						*/
            EPOCH_ROUTING_SOURCE_VIDEO_MODE = VIDEO_SCALER_MODE,/**< Use this property to change the video mode that scaler should be set to.
							USe the macro SET_EPOCH_SCALER_MODE when using this property, as this macro 
							would allow you to select which one of the scaler blocks video mode should be updated.
							*/
            EPOCH_AVAIL_VIDEO_SCALER_COUNT = 85,/**< This property would return available scaler processing block available on the card.*/
            EPOCH_ENUM_AVAIL_VIDEO_SCALERS_ID = 86,/**< You can enumerate the available scaler processing block available on the card using this property.
					You pass in the index value as input parameter to get the scaler id that should be used.
					Applications are recommended to use this property to query the available scaler id's 
					rather than hardcoding a scaler id. As the scaler id's that you can use would vary based on
					whether you have VPS0 or VPS1 boards loaded on the base board.
					*/
            EPOCH_ALLOCATE_VIDEO_SCALER = 87,	/**< This is just a helper property for applications who need to use more than one scaler
					and just wants to query the next available scaler from the driver pool, rather than hardcoding 
					each thread to use a particular scaler.
					Allocate a free scaler from the available scaler pool.
					User has got the option to specify whether they want to use the scaler for 
					use with a single link or dual link stream */
            EPOCH_RELEASE_VIDEO_SCALER = 88, /**< Release the previously allocated scaler processing block back to the free pool.
					If the user passes in a value of 0, all the allocated scaler blocks in the driver are released.
					So effectively
					*/
            EPOCH_DMA_CARDMEMORY_PITCH = 89,
            EPOCH_OUTPUT_CHANNEL_AV_OFFSET = 90,
            EPOCH_SCALER_CHANNEL_MUX_MODE = 91,
            EPOCH_INPUT_CHANNEL_AV_OFFSET = 92,
            EPOCH_AUDIOOUTPUT_MANUAL_UCZV_GENERATION = 93,	/* ASI firmware only */
            EPOCH_SAMPLE_RATE_CONVERTER_BYPASS = 94,		/** bypasses the sample rate converter for AES audio; only turn on for Dolby-E support
												 *	pass in a flag to signal which audio stereo pair should be bypassed:
												 *	bit 0:	AES channels 0 and 1
												 *	bit 1:	AES channels 2 and 3
												 *	bit 2:	AES channels 4 and 5
												 *	bit 3:	AES channels 6 and 7
												 *	For example: bypass the sample rate converter for channels 0 to 3: flag = 0x3;	*/
            EPOCH_GET_PRODUCT_ID = 95,						/* returns the enum for the firmware type EEpochFirmwareProductID */
            EPOCH_GENLOCK_IS_LOCKED = 96,
            EPOCH_DVB_ASI_OUTPUT_PACKET_COUNT = 97,			/* ASI firmware only */
            EPOCH_DVB_ASI_OUTPUT_BIT_RATE = 98,				/* ASI firmware only */
            EPOCH_DVB_ASI_DUPLICATE_OUTPUT_A = 99,			/* ASI firmware only */
            EPOCH_DVB_ASI_DUPLICATE_OUTPUT_B = 100,			/* ASI firmware only */
            EPOCH_SCALER_HORIZONTAL_FLIP = 101,				/* see SideBySide_3D sample application */
            EPOCH_CONNECTOR_DIRECTION = 102,					/* see application notes */
            EPOCH_AUDIOOUTPUT_VALIDITY_BITS = 103,			/* ASI firmware only */
            EPOCH_SIZEOF_DRIVER_ALLOCATED_MEMORY = 104,	/* video buffer allocated in Kernel space; accessible in userland via system_buffer_map() */
            INVALID_VIDEO_MODE_FLAG = 105,				/* returns the enum for VID_FMT_INVALID that this SDK/Driver was compiled with;
												it changed between 5.9.x.x and 5.10.x.x driver branch and has to be handled differently for
												each driver if the application wants to use the VID_FMT_INVALID flag and support both driver branches */
            EPOCH_VIDEO_INPUT_VPID = 106,					/* returns the VPID for the current video input signal */
            EPOCH_LOW_LATENCY_DMA = 107,					/* deprecated; use new feature EPOCH_SUBFIELD_INPUT_INTERRUPTS instead */
            EPOCH_VIDEO_INPUT_RGB_DATA_RANGE = 108,
            EPOCH_DVB_ASI_OUTPUT_PACKET_SIZE = 109,		/* firmware supports either 188 or 204 bytes per ASI packet; set to either
													enum_blue_dvb_asi_packet_size_188_bytes or
													enum_blue_dvb_asi_packet_size_204_bytes */
            EPOCH_SUBFIELD_INPUT_INTERRUPTS = 110,		/* similar to the EPOCH_LOW_LATENCY_DMA card feature, but this doesn't influence the DMA;
													it simply adds interrupts between the frame/field interrupts that trigger when a corresponding
													video chunk has been captured
													required minimum driver: 5.10.1.8*/
            EPOCH_AUDIOOUTPUT_METADATA_SETTINGS = 111,	/* Use the EAudioMetaDataSettings enumerator to change the audio output metadata settings */
            EPOCH_HD_SDI_TRANSPORT = 112,				/* output only: available modes are defined in the enum EHdSdiTransport; for inputs see EPOCH_HD_SDI_TRANSPORT_INPUT  */
            CARD_FEATURE_STREAM_INFO = 113,				/* only supported from driver 5.10.2.x; info on how many in/out SDI/ASI streams are supported */
            CARD_FEATURE_CONNECTOR_INFO = 114,			/* only supported from driver 5.10.2.x; info on which connectors are supported: SDI in/out, AES, RS422, LTC, GPIO */
            EPOCH_HANC_INPUT_FLAGS = 115,				/* this property can be queried to test flags being set in the HANC space (e.g. HANC_FLAGS_IS_ARRI_RECORD_FLAG_SET) */
            EPOCH_INPUT_VITC = 116,						/* this property retrieves the current input VITC timecode; set .vt = VT_UI8 as this is a 64bit value;
													only supported by SuperNova 2i/2o firmware version 75 and above */
            EPOCH_RAW_VIDEO_INPUT_TYPE = 117,			/* specifies if the raw/bayer input is ARRI 10/12 bit or Weisscam; set to 0 to revert back to normal SDI mode */
            EPOCH_PCIE_CONFIG_INFO = 118,				/* only supported from driver 5.10.2.x; provides info on PCIE maximum payload size and maximum read request siize */
            EPOCH_4K_QUADLINK_CHANNEL = 119,			/* use this property to set the 4K quadrant number for the current channel in 4K output mode; quadrant numbers are 1 - 4 */
            EXTERNAL_LTC_SOURCE_SELECTION = 120,		/* use the enum EBlueExternalLtcSource to set the input source for the external LTC */
            EPOCH_HD_SDI_TRANSPORT_INPUT = 121,			/* can only be queried; return values are defined in the enum EHdSdiTransport */

            VIDEO_CARDPROPERTY_INVALID = 1000
        };

        // more definitions in header file...
        // lazy to do it now... ;)
    }
}
