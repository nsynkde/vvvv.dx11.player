using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Timers;
using BluePlaybackNetLib;
using VVVV.Core.Logging;
using SlimDX.Direct3D9;



namespace VVVV.Nodes.Bluefish
{
	class Source : IDisposable
	{
        [DllImport("msvcrt.dll", EntryPoint = "memset", CallingConvention = CallingConvention.Cdecl, SetLastError = false)]
        public static extern IntPtr MemSet(IntPtr dest, int c, IntPtr count);

        BluePlaybackNet FDevice;
		public ModeRegister.Mode Mode {get; private set;}

		int FFrameIndex = 0;
        ILogger FLogger;
        WorkerThread worker = new WorkerThread();

		
		public int Width
		{
			get
			{
                return (int)FDevice.BluePlaybackGetPixelsPerLine();
			}
		}

		public int Height
        {
            get
            {
                return (int)FDevice.BluePlaybackGetVideoLines();
            }
		}

        public int Pitch
        {
            get
            {
                return (int)FDevice.BluePlaybackGetBytesPerLine();
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
                FDevice.BluePlaybackSetCardProperty((int)BlueFish_h.EBlueCardProperty.VIDEO_MODE, (uint)value);
            }
        }

        public uint Channel
        {
            get
            {
                ulong channel;
                FDevice.BluePlaybackQueryULongCardProperty((int)BlueFish_h.EBlueCardProperty.DEFAULT_VIDEO_OUTPUT_CHANNEL, out channel);
                return (uint)channel;
            }

            set
            {
                FDevice.BluePlaybackSetCardProperty((int)BlueFish_h.EBlueCardProperty.DEFAULT_VIDEO_OUTPUT_CHANNEL, value);
            }
        }

        public BlueFish_h.EConnectorSignalColorSpace OutputColorSpace
        {
            get
            {
                ulong colorSpace;
                FDevice.BluePlaybackQueryULongCardProperty((int)BlueFish_h.EBlueCardProperty.VIDEO_OUTPUT_SIGNAL_COLOR_SPACE, out colorSpace);
                return (BlueFish_h.EConnectorSignalColorSpace)colorSpace;
            }
            set
            {
                FDevice.BluePlaybackSetCardProperty((int)BlueFish_h.EBlueCardProperty.VIDEO_OUTPUT_SIGNAL_COLOR_SPACE, (uint)value);
            }
        }

        public bool DualLink
        {
            get
            {
                ulong dualLink;
                FDevice.BluePlaybackQueryULongCardProperty((int)BlueFish_h.EBlueCardProperty.VIDEO_DUAL_LINK_OUTPUT, out dualLink);
                return dualLink!=0;
            }
            set
            {
                uint dualLink = (uint)(value?1:0);
                FDevice.BluePlaybackSetCardProperty((int)BlueFish_h.EBlueCardProperty.VIDEO_DUAL_LINK_OUTPUT, dualLink);
            }
        }

        public BlueFish_h.EDualLinkSignalFormatType DualLinkSignalFormat
        {
            get
            {
                ulong dualLinkFormat;
                FDevice.BluePlaybackQueryULongCardProperty((int)BlueFish_h.EBlueCardProperty.VIDEO_DUAL_LINK_OUTPUT_SIGNAL_FORMAT_TYPE, out dualLinkFormat);
                return (BlueFish_h.EDualLinkSignalFormatType)dualLinkFormat;
            }
            set
            {
                FDevice.BluePlaybackSetCardProperty((int)BlueFish_h.EBlueCardProperty.VIDEO_DUAL_LINK_OUTPUT_SIGNAL_FORMAT_TYPE, (uint)value);
            }

        }

        public BlueFish_h.EPreDefinedColorSpaceMatrix ColorMatrix
        {
            get
            {
                ulong colorMatrix;
                FDevice.BluePlaybackQueryULongCardProperty((int)BlueFish_h.EBlueCardProperty.VIDEO_PREDEFINED_COLOR_MATRIX, out colorMatrix);
                return (BlueFish_h.EPreDefinedColorSpaceMatrix)colorMatrix;
            }
            set
            {
                FDevice.BluePlaybackSetCardProperty((int)BlueFish_h.EBlueCardProperty.VIDEO_PREDEFINED_COLOR_MATRIX, (uint)value);
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


        public Source(BluePlaybackNet device, BlueFish_h.EVideoMode mode, bool useDeviceCallbacks,ILogger FLogger)
		{
			this.Initialise(device, mode, useDeviceCallbacks,FLogger);
		}

        public void Initialise(BluePlaybackNet device, BlueFish_h.EVideoMode mode, bool useDeviceCallbacks, ILogger FLogger)
		{
			try
			{
				//--
				//attach to device
				//
				if (device == null)
					throw (new Exception("No device"));

				FDevice = device;

                FVideoMode = mode;

                // configure card
                FDevice.BluePlaybackInterfaceStart();
                int memFormat = (int)BlueFish_h.EMemoryFormat.MEM_FMT_BGRA;

                int updateMethod = (int)BlueFish_h.EUpdateMethod.UPD_FMT_FRAME;
                FDevice.BluePlaybackInterfaceConfig(1, // Device Number
                                                    0, // output channel
                                                    (int)mode, // video mode //VID_FMT_PAL=0
                                                    memFormat, // memory format //MEM_FMT_ARGB_PC=6 //MEM_FMT_BGR=9
                                                    updateMethod, // update type (frame, field) //UPD_FMT_FRAME=1
                                                    0, // video destination
                                                    0, // audio destination
                                                    0  // audio channel mask
                                                    );

				//--
				//scheduled playback
				if (useDeviceCallbacks == true)
				{
                    /*
					FOutputDevice.SetScheduledFrameCompletionCallback(this);
					this.FFrameIndex = 0;
					for (int i = 0; i < (int)this.Framerate; i++)
					{
						ScheduleFrame(true);
					}
					FOutputDevice.StartScheduledPlayback(0, 100, 1.0);
                        */
				}
				//
				//--
                this.FLogger = FLogger;
				FRunning = true;
			}
			catch (Exception e)
			{
				this.FRunning = false;
				throw;
			}
		}

		public void Stop()
        {

            // stop device, black frame...

			if (!FRunning)
				return;

			//stop new frames from being scheduled
			FRunning = false;

            FDevice.BluePlaybackInterfaceStop();
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
                        int result = FDevice.BluePlaybackInterfaceRender(ptr);

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
                int result = FDevice.BluePlaybackInterfaceRender(data);

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
                int result = FDevice.BluePlaybackInterfaceRender(rect.Data.DataPointer);
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
            FDevice.BluePlaybackInterfaceWaitSync();
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
}

