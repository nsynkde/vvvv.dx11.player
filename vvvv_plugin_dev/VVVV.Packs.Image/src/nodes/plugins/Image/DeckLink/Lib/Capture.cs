using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using DeckLinkAPI;
using System.Threading;
using System.Runtime.InteropServices;

// first reference : http://stackoverflow.com/questions/6355930/blackmagic-sdk-in-c-sharp

namespace VVVV.Nodes.DeckLink
{
	class Capture : IDisposable, IDeckLinkInputCallback
	{
		IDeckLinkInput FDevice;
		_BMDDisplayMode FMode;
		_BMDVideoInputFlags FFlags;
		IDeckLinkDisplayMode FDisplayMode;
		_BMDPixelFormat FPixelFormat = _BMDPixelFormat.bmdFormat8BitYUV;
		IntPtr FData;
		public bool Reinitialise { get; private set;}
		public bool FreshData { get; private set; }
		public int Width { get; private set; }
		public int Height { get; private set; }
		public Mutex Lock = new Mutex();
		public bool Ready { get; private set; }

        public delegate void FrameCallbackDelegate(IntPtr data, int deviceIdx);
        public FrameCallbackDelegate FFrameCallback;
        private int FDeviceIdx;
        public int DeviceIdx
        {
            set
            {
                FDeviceIdx = value;
            }
        }

		public Capture()
		{
			Reinitialise = false;
			FreshData = false;
			Width = 0;
			Height = 0;
			Ready = false;
		}

		public void Open(DeviceRegister.DeviceIndex device, _BMDDisplayMode mode, _BMDVideoInputFlags flags)
		{
			if (Ready)
				Close();

			//this.Lock.WaitOne();
			try
			{
				if (device == null)
					throw (new Exception("No device selected"));

				IDeckLink rawDevice = DeviceRegister.Singleton.GetDeviceHandle(device.Index);
				FDevice = rawDevice as IDeckLinkInput;
				FMode = mode;
				FFlags = flags;

				if (FDevice == null)
					throw (new Exception("No input device connected"));

				_BMDDisplayModeSupport displayModeSupported;

				FDevice.DoesSupportVideoMode(FMode, FPixelFormat, flags, out displayModeSupported, out FDisplayMode);

				Width = FDisplayMode.GetWidth();
				Height = FDisplayMode.GetHeight();

				FDevice.EnableVideoInput(FMode, FPixelFormat, FFlags);
				FDevice.SetCallback(this);
				FDevice.StartStreams();

				Reinitialise = true;
				Ready = true;
				FreshData = false;
			}
			catch (Exception e)
			{
				Ready = false;
				Reinitialise = false;
				FreshData = false;
				throw;
			}
			finally
			{
				//this.Lock.ReleaseMutex();
			}
		}

		public void Close()
		{
			//this.Lock.WaitOne();
			try
			{
                if (!Ready)
                {
                    //this.Lock.ReleaseMutex();
                    return;
                }

				Ready = false;
                FDevice.SetCallback(null);
                FDevice.StopStreams();
                FDevice.DisableVideoInput();
                 
			}
			finally
			{
                //this.Lock.ReleaseMutex();
			}

		}

		public void VideoInputFormatChanged(_BMDVideoInputFormatChangedEvents notificationEvents, IDeckLinkDisplayMode newDisplayMode, _BMDDetectedVideoInputFormatFlags detectedSignalFlags)
		{
			Reinitialise = true;
		}

		public void VideoInputFrameArrived(IDeckLinkVideoInputFrame videoFrame, IDeckLinkAudioInputPacket audioPacket)
		{
            this.Lock.WaitOne();
			try
			{
				videoFrame.GetBytes(out FData);
                if (FFrameCallback != null)
                {
                    FFrameCallback(FData, FDeviceIdx);
                }
                System.Runtime.InteropServices.Marshal.ReleaseComObject(videoFrame);
                FreshData = true;
			}
			catch
			{

			}
			finally
			{
                this.Lock.ReleaseMutex();
			}
		}

		public void Reinitialised()
		{
			this.Reinitialise = false;
		}

		public void Updated()
		{
			this.FreshData = false;
		}

		public IntPtr Data
		{
			get
			{
				return FData;
			}
		}

		public int BytesPerFrame
		{
			get
			{
				return Width / 2 * Height * 4;
			}
		}

		public int AvailableFrameCount
		{
			get
			{
				if (!Ready)
					return 0;

				uint count = 0;
				FDevice.GetAvailableVideoFrameCount(out count);
				return (int)count;
			}
		}

		public void Flush()
		{
			if (!Ready)
				return;
			FDevice.FlushStreams();
		}

		public void Dispose()
		{
			Close();
		}
	}
}
