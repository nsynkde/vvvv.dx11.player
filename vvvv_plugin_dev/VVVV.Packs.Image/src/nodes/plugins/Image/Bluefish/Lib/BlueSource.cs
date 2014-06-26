using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Timers;
using VVVV.Core.Logging;
using SlimDX.Direct3D9;



namespace VVVV.Nodes.Bluefish
{
	class BluefishSource : IDisposable
	{

        private IntPtr pBluePlayback;
        private ILogger FLogger;
        private WorkerThread worker = new WorkerThread();

		
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

		public delegate void FrameServeHandler(IntPtr data);
		/// <summary>
		/// Callback for scheduled playback
		/// </summary>
		public event FrameServeHandler NewFrame;
		void OnNewFrame(IntPtr data)
		{
			if (NewFrame != null)
				NewFrame(data);
		}


        public BluefishSource(uint device, uint channel, BlueFish_h.EVideoMode mode, BlueFish_h.EMemoryFormat format, ILogger FLogger)
		{
			this.Initialise(device, channel, mode, format, FLogger);
		}

        public void Initialise(uint device, uint channel, BlueFish_h.EVideoMode mode, BlueFish_h.EMemoryFormat format, ILogger FLogger)
		{
			try
			{

                pBluePlayback = BluePlaybackNativeInterface.BluePlaybackCreate();
                //--
                //attach to device
                //
                if (pBluePlayback == null)
                    throw (new Exception("No device"));

                FVideoMode = mode;

                // configure card
                //int memFormat = (int)BlueFish_h.EMemoryFormat.MEM_FMT_RGBA;

                int updateMethod = (int)BlueFish_h.EUpdateMethod.UPD_FMT_FRAME;
                int err = BluePlaybackNativeInterface.BluePlaybackConfig(
                                                    pBluePlayback,
                                                    (int)device+1, // Device Number
                                                    (int)channel, // output channel
                                                    (int)mode, // video mode //VID_FMT_PAL=0
                                                    (int)format, // memory format //MEM_FMT_ARGB_PC=6 //MEM_FMT_BGR=9
                                                    updateMethod, // update type (frame, field) //UPD_FMT_FRAME=1
                                                    0, // video destination
                                                    0, // audio destination
                                                    0  // audio channel mask
                                                    );
                BluePlaybackNativeInterface.BluePlaybackStart(pBluePlayback);

                if (err != 0)
                {
                    throw new Exception("Error on blue interface config");
                }

                this.FLogger = FLogger;
				FRunning = true;

                FLogger.Log(LogType.Message, "config device");
			}
			catch (Exception e)
			{
				this.FRunning = false;
				throw e;
			}
		}

		public void Stop()
        {

            // stop device, black frame...

			if (!FRunning)
				return;

			//stop new frames from being scheduled
			FRunning = false;

            BluePlaybackNativeInterface.BluePlaybackStop(pBluePlayback);
		}

		public Object LockBuffer = new Object();

		public void SendFrame(byte[] buffer)
		{
            worker.Perform(() =>
            {
                unsafe
                {
                    fixed (byte* p = buffer)
                    {
                        IntPtr ptr = (IntPtr)p;
                    
                        // send bytes blocking
                        int result = BluePlaybackNativeInterface.BluePlaybackRender(pBluePlayback, ptr);

                        if (result < 0)
                        {
                            Console.WriteLine("error writing bytes");
                        }
                    } 
                }
            });

            

            

		}

        public void SendFrame(IntPtr data)
        {
            worker.Perform(() =>
            {
                int result = BluePlaybackNativeInterface.BluePlaybackRender(pBluePlayback, data);

                if (result < 0)
                {
                    FLogger.Log(LogType.Error, "error writing bytes");
                }
            });

        }

        public void SendFrame(Texture texture)
        {
            worker.Perform(() =>
            {

                var rect = texture.LockRectangle(0, LockFlags.ReadOnly);
                int result = BluePlaybackNativeInterface.BluePlaybackRender(pBluePlayback, rect.Data.DataPointer);
                texture.UnlockRectangle(0);

                if (result < 0)
                {
                    FLogger.Log(LogType.Error, "error writing bytes");
                }
            });

        }

        public void WaitSync()
        {
            //worker.BlockUntilEmpty();
            BluePlaybackNativeInterface.BluePlaybackWaitSync(pBluePlayback);
        }

        public bool DoneLastFrame()
        {
            return worker.QueueSize == 0;
        }

		public void Dispose()
		{
			Stop();
		}

        
		void ScheduleFrame(bool preRoll)
		{
			if (!preRoll && !FRunning)
			{
				return;
			}
            /*
			IntPtr data;
			FVideoFrame.GetBytes(out data);
			OnNewFrame(data);


			long displayTime = FFrameIndex * FFrameDuration;
			FOutputDevice.ScheduleVideoFrame(FVideoFrame, displayTime, FFrameDuration, FFrameTimescale);
			FFrameIndex++;
            */
		}

		public void ScheduledPlaybackHasStopped()
		{
		}

	}

    internal class BluePlaybackNativeInterface
    {
        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern IntPtr BluePlaybackCreate();

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

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern void BluePlaybackStop(IntPtr pBluePlaybackObject);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern int BluePlaybackWaitSync(IntPtr pBluePlaybackObject);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern int BluePlaybackRender(IntPtr pBluePlaybackObject, IntPtr pBuffer);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern void BluePlaybackDestroy(IntPtr pBluePlaybackObject);

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
    }
}

