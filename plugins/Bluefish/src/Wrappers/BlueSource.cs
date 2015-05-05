using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Timers;
using VVVV.Core.Logging;
using System.Runtime.ExceptionServices;




namespace VVVV.Nodes.Bluefish
{
	class BluefishSource : IDisposable
	{
        private IntPtr pBlueRenderer;
        private IntPtr pBluePlayback;
        private ILogger FLogger;
        private int FDeviceNumber;
		
		public int Width
		{
			get
			{
                return (int)BluePlaybackNativeInterface.BluePlaybackGetPixelsPerLine(pBluePlayback);
			}
		}

		public int Height
        {
            get
            {
                return (int)BluePlaybackNativeInterface.BluePlaybackGetVideoLines(pBluePlayback);
            }
		}

        public int Pitch
        {
            get
            {
                return (int)BluePlaybackNativeInterface.BluePlaybackGetBytesPerLine(pBluePlayback);
            }
        }

		bool FRunning = false;
		public bool Running
		{
			get
			{
				return FRunning;
			}
		}

        BlueFish_h.EVideoMode FVideoMode;
        public BlueFish_h.EVideoMode VideoMode
        {
            get 
            {
                return FVideoMode;
            }

            set
            {
                FVideoMode = value;
                BluePlaybackNativeInterface.BluePlaybackSetCardProperty(pBluePlayback,(int)BlueFish_h.EBlueCardProperty.VIDEO_MODE, (uint)value);
            }
        }

        public uint Channel
        {
            get
            {
                ulong channel;
                BluePlaybackNativeInterface.BluePlaybackQueryULongCardProperty(pBluePlayback,(int)BlueFish_h.EBlueCardProperty.DEFAULT_VIDEO_OUTPUT_CHANNEL, out channel);
                return (uint)channel;
            }

            set
            {
                BluePlaybackNativeInterface.BluePlaybackSetCardProperty(pBluePlayback,(int)BlueFish_h.EBlueCardProperty.DEFAULT_VIDEO_OUTPUT_CHANNEL, value);
            }
        }

        public BlueFish_h.EConnectorSignalColorSpace OutputColorSpace
        {
            get
            {
                ulong colorSpace;
                BluePlaybackNativeInterface.BluePlaybackQueryULongCardProperty(pBluePlayback,(int)BlueFish_h.EBlueCardProperty.VIDEO_OUTPUT_SIGNAL_COLOR_SPACE, out colorSpace);
                return (BlueFish_h.EConnectorSignalColorSpace)colorSpace;
            }
            set
            {
                BluePlaybackNativeInterface.BluePlaybackSetCardProperty(pBluePlayback,(int)BlueFish_h.EBlueCardProperty.VIDEO_OUTPUT_SIGNAL_COLOR_SPACE, (uint)value);
            }
        }

        public bool DualLink
        {
            get
            {
                ulong dualLink;
                BluePlaybackNativeInterface.BluePlaybackQueryULongCardProperty(pBluePlayback,(int)BlueFish_h.EBlueCardProperty.VIDEO_DUAL_LINK_OUTPUT, out dualLink);
                return dualLink!=0;
            }
            set
            {
                uint dualLink = (uint)(value?1:0);
                BluePlaybackNativeInterface.BluePlaybackSetCardProperty(pBluePlayback,(int)BlueFish_h.EBlueCardProperty.VIDEO_DUAL_LINK_OUTPUT, dualLink);
            }
        }

        public BlueFish_h.EDualLinkSignalFormatType DualLinkSignalFormat
        {
            get
            {
                ulong dualLinkFormat;
                BluePlaybackNativeInterface.BluePlaybackQueryULongCardProperty(pBluePlayback,(int)BlueFish_h.EBlueCardProperty.VIDEO_DUAL_LINK_OUTPUT_SIGNAL_FORMAT_TYPE, out dualLinkFormat);
                return (BlueFish_h.EDualLinkSignalFormatType)dualLinkFormat;
            }
            set
            {
                BluePlaybackNativeInterface.BluePlaybackSetCardProperty(pBluePlayback,(int)BlueFish_h.EBlueCardProperty.VIDEO_DUAL_LINK_OUTPUT_SIGNAL_FORMAT_TYPE, (uint)value);
            }

        }

        public BlueFish_h.EPreDefinedColorSpaceMatrix ColorMatrix
        {
            get
            {
                ulong colorMatrix;
                BluePlaybackNativeInterface.BluePlaybackQueryULongCardProperty(pBluePlayback,(int)BlueFish_h.EBlueCardProperty.VIDEO_PREDEFINED_COLOR_MATRIX, out colorMatrix);
                return (BlueFish_h.EPreDefinedColorSpaceMatrix)colorMatrix;
            }
            set
            {
                BluePlaybackNativeInterface.BluePlaybackSetCardProperty(pBluePlayback,(int)BlueFish_h.EBlueCardProperty.VIDEO_PREDEFINED_COLOR_MATRIX, (uint)value);
            }

        }

        public string SerialNumber
        {
            get
            {
                return Marshal.PtrToStringAnsi(BluePlaybackNativeInterface.BluePlaybackGetSerialNumber(pBluePlayback));
            }
        }

        public IntPtr DX11Device
        {
            get
            {
                return BluePlaybackNativeInterface.BlueRendererGetDX11Device(pBluePlayback);
            }
        }

        public IntPtr SharedHandle
        {
            set
            {
                BluePlaybackNativeInterface.BlueRendererSetSharedHandle(pBlueRenderer,value);
            }
        }


        public BluefishSource(uint device, uint channel, BlueFish_h.EVideoMode mode, BlueFish_h.EMemoryFormat format, ulong tex_handle, int num_render_target_buffers, int num_read_back_buffers, int num_bluefish_buffers, ILogger FLogger)
		{
			this.Initialise(device, channel, mode, format, tex_handle, num_render_target_buffers, num_read_back_buffers, num_bluefish_buffers, FLogger);
		}

        [DllImport("kernel32.dll", CharSet = CharSet.Auto)]
        public static extern void OutputDebugString(string message);

        public void Initialise(uint device, uint channel, BlueFish_h.EVideoMode mode, BlueFish_h.EMemoryFormat format, ulong tex_handle, int num_render_target_buffers, int num_read_back_buffers, int num_bluefish_buffers, ILogger FLogger)
		{
			try
            {
                //pBluePlayback = BluePlaybackNativeInterface.BluePlaybackCreate();
                OutputDebugString("Creating renderer");
                pBlueRenderer = BluePlaybackNativeInterface.BlueRendererCreate((IntPtr)tex_handle, (int)format, num_render_target_buffers, num_read_back_buffers, num_bluefish_buffers);
                if (pBlueRenderer == (IntPtr)0)
                {
                    throw new Exception("Couldn't create renderer");
                }
                OutputDebugString("Got renderer: " + pBlueRenderer);
                FLogger.Log(LogType.Message, "Got renderer: " + pBlueRenderer);
                pBluePlayback = BluePlaybackNativeInterface.BlueRendererGetPlaybackDevice(pBlueRenderer);
                FLogger.Log(LogType.Message, "Got device: " + pBluePlayback);
                OutputDebugString("Got device: " + pBluePlayback);
                //--
                //attach to device
                //
                if (pBluePlayback == null)
                    throw (new Exception("No device"));

                FVideoMode = mode;

                // configure card
                //int memFormat = (int)BlueFish_h.EMemoryFormat.MEM_FMT_RGBA;
                FDeviceNumber = (int)device;
                int updateMethod = (int)BlueFish_h.EUpdateMethod.UPD_FMT_FRAME;
                int err = BluePlaybackNativeInterface.BluePlaybackConfig(
                                                    pBluePlayback,
                                                    (int)device, // Device Number
                                                    (int)channel, // output channel
                                                    (int)mode, // video mode //VID_FMT_PAL=0
                                                    (int)format, // memory format //MEM_FMT_ARGB_PC=6 //MEM_FMT_BGR=9
                                                    updateMethod, // update type (frame, field) //UPD_FMT_FRAME=1
                                                    0, // video destination
                                                    0, // audio destination
                                                    0  // audio channel mask
                                                    );
                OutputDebugString("Config set on blueplayback");
                BluePlaybackNativeInterface.BluePlaybackStart(pBluePlayback);
                OutputDebugString("blueplayback started");

                if (err != 0)
                {
                    throw new Exception("Error on blue interface config");
                }

                this.FLogger = FLogger;
				FRunning = true;
			}
			catch (Exception e)
			{
				this.FRunning = false;
				throw e;
			}
		}

        public void OnPresent()
        {
            if (!FRunning)
                return;

            BluePlaybackNativeInterface.BlueRendererOnPresent(pBlueRenderer);
            BluePlaybackNativeInterface.BluePlaybackWaitGlobalSync(FDeviceNumber);
        }

        public double AvgDuration
        {
            get
            {
                if (!FRunning)
                    return 0;
                return BluePlaybackNativeInterface.BlueRendererGetAvgDuration(pBlueRenderer);
            }
        }

        public double MaxDuration
        {
            get
            {
                if (!FRunning)
                    return 0;
                return BluePlaybackNativeInterface.BlueRendererGetMaxDuration(pBlueRenderer);
            }
        }

        public void Stop()
        {

            // stop device, black frame...

            if (!FRunning)
                return;

            //stop new frames from being scheduled
            FRunning = false;

            BluePlaybackNativeInterface.BlueRendererStop(pBlueRenderer);
        }

		public void Dispose()
		{
			Stop();
            BluePlaybackNativeInterface.BlueRendererDestroy(pBlueRenderer);
		}

	}

    internal class BluePlaybackNativeInterface
    {
        /*[DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern IntPtr BluePlaybackCreate();*/

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern Int32 BluePlaybackConfig(IntPtr pBluePlaybackObject, Int32 inDevNo,
                                                                                    Int32 inChannel,
                                                                                    Int32 inVidFmt,
                                                                                    Int32 inMemFmt,
                                                                                    Int32 inUpdFmt,
                                                                                    Int32 inVideoDestination,
                                                                                    Int32 inAudioDestination,
                                                                                    Int32 inAudioChannelMask);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern void BluePlaybackStart(IntPtr pBluePlaybackObject);

        /*[DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern void BluePlaybackStop(IntPtr pBluePlaybackObject);*/

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern int BluePlaybackWaitSync(IntPtr pBluePlaybackObject);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern int BluePlaybackWaitGlobalSync(int deviceNum);

        /*[DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern int BluePlaybackUpload(IntPtr pBluePlaybackObject, IntPtr pBuffer);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern int BluePlaybackRenderNext(IntPtr pBluePlaybackObject);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern void BluePlaybackDestroy(IntPtr pBluePlaybackObject);*/

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern int BluePlaybackSetCardProperty(IntPtr pBluePlaybackObject, int nProp, uint nValue);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern int BluePlaybackSetCardPropertyInt64(IntPtr pBluePlaybackObject, int nProp, Int64 nValue);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern int BluePlaybackQueryULongCardProperty(IntPtr pBluePlaybackObject, int nProp, out ulong outPropValue);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern int BluePlaybackQueryUIntCardProperty(IntPtr pBluePlaybackObject, int nProp, out uint outPropValue);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern ulong BluePlaybackGetMemorySize(IntPtr pBluePlaybackObject);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern IntPtr BluePlaybackGetSerialNumber(IntPtr pBluePlaybackObject);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern string BluePlaybackBlueVelvetVersion();

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern int BluePlaybackGetDeviceCount(IntPtr pBluePlaybackObject);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern ulong BluePlaybackGetPixelsPerLine(IntPtr pBPHandle);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern ulong BluePlaybackGetVideoLines(IntPtr pBPHandle);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern ulong BluePlaybackGetBytesPerLine(IntPtr pBPHandle);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern ulong BluePlaybackGetBytesPerFrame(IntPtr pBPHandle);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern IntPtr BlueRendererCreate(IntPtr tex_handle, int outFormat, int num_render_target_buffers, int num_read_back_buffers, int num_bluefish_buffers);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern void BlueRendererDestroy(IntPtr renderer);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern void BlueRendererStop(IntPtr renderer);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern IntPtr BlueRendererGetPlaybackDevice(IntPtr renderer);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern IntPtr BlueRendererGetDX11Device(IntPtr renderer);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern void BlueRendererSetSharedHandle(IntPtr renderer, IntPtr tex_handle);
        
        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern IntPtr BlueRendererOnPresent(IntPtr renderer);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern double BlueRendererGetAvgDuration(IntPtr renderer);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern double BlueRendererGetMaxDuration(IntPtr renderer);
    }
}

