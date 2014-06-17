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

		int FWidth = 0;
		public int Width
		{
			get
			{
				return FWidth;
			}
		}

		int FHeight = 0;
		public int Height
		{
			get
			{
				return FHeight;
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

		long FFrameDuration = 0;
		long FFrameTimescale = 0;
		public double Framerate
		{
			get
			{
				return (double)FFrameTimescale / (double)FFrameDuration;
			}
		}

		uint FFramesInBuffer = 0;
		public int FramesInBuffer
		{
			get
			{
				return (int) FFramesInBuffer;
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
			//Stop();

			try
			{
				//WorkerThread.Singleton.PerformBlocking(() => {

					//--
					//attach to device
					//
					if (device == null)
						throw (new Exception("No device"));
					if (mode == null)
						throw (new Exception("No mode selected"));

					FDevice = device;


                    //VID_FMT_PAL=0
                    //VID_FMT_1080I_5000=8
                    //VID_FMT_1080P_2500=11

                    int videoMode = (int)mode;

                    //MEM_FMT_ARGB_PC=6
                    //MEM_FMT_BGR=9
                    int memFormat = (int)BlueFish_h.EMemoryFormat.MEM_FMT_BGRA;

                    int updateMethod = (int)BlueFish_h.EUpdateMethod.UPD_FMT_FRAME;

                    // configure card
                    FDevice.BluePlaybackInterfaceStart();
                    FDevice.BluePlaybackInterfaceConfig(1, // Device Number
                                                                0, // output channel
                                                                videoMode, // video mode //VID_FMT_PAL=0
                                                                memFormat, // memory format //MEM_FMT_ARGB_PC=6 //MEM_FMT_BGR=9
                                                                updateMethod, // update type (frame, field) //UPD_FMT_FRAME=1
                                                                0, // video destination
                                                                0, // audio destination
                                                                0  // audio channel mask
                                                                ); //Dev 1, Output channel A, PAL, BGRA, FRAME_MODE, not used, not used, not used


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
				//});
			}
			catch (Exception e)
			{
				this.FWidth = 0;
				this.FHeight = 0;
				this.FRunning = false;
				throw;
			}
		}

		public void Stop()
		{
            FLogger.Log(LogType.Error, "stopping");

			if (!FRunning)
				return;

			//stop new frames from being scheduled
			FRunning = false;

            byte[] black = new byte[1920 * 1080 * 4];
            unsafe
            {
                fixed (byte* p = black)
                {
                    IntPtr ptr = (IntPtr)p;
                    MemSet(ptr, 0, (IntPtr)(1920 * 1080 * 4));
                    int result = FDevice.BluePlaybackInterfaceRender(ptr);
                }
            }

            // stop device, black frame...
            FDevice.BluePlaybackInterfaceDestroy();
		}

		void FillBlack(IntPtr data)
		{
			MemSet(data, 0, (IntPtr)(Mode.CompressedWidth * Mode.Height * 4));
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

