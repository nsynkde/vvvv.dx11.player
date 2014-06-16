#region usings
using System;
using System.ComponentModel.Composition;
using System.Runtime.InteropServices;

using SlimDX;
using SlimDX.Direct3D9;
using VVVV.Core.Logging;
using VVVV.PluginInterfaces.V1;
using VVVV.PluginInterfaces.V2;
using VVVV.PluginInterfaces.V2.EX9;
using VVVV.Utils.VColor;
using VVVV.Utils.VMath;
using VVVV.Utils.SlimDX;
using BluePlaybackNetLib;

#endregion usings

//here you can change the vertex type
using VertexType = VVVV.Utils.SlimDX.TexturedVertex;
using System.Threading;
using System.Diagnostics;

namespace VVVV.Nodes.Bluefish
{
    
    #region PluginInfo
    [PluginInfo(Name = "VideoOut",
                Category = "Bluefish",
                Help = "Given a texture handle, will push graphic to Bluefish device",
                Tags = "",
                Author = "nsynk",
                AutoEvaluate = true)]
    #endregion PluginInfo
	public class VideoOut : IPluginEvaluate, IPartImportsSatisfiedNotification, IDisposable
    {
		enum SyncLoop
		{
			Bluefish,
			VVVV,
			Sync
		}

		class Instance : IDisposable
		{
			public Source Source;
			public ReadTexture ReadTexture;
			byte[] FBuffer;
            int frame = 0;

            ILogger FLogger;

			public Instance(int deviceID, string modeString, uint textureHandle, EnumEntry format, EnumEntry usage, SyncLoop syncLoop, ILogger FLogger)
			{
                this.FLogger = FLogger;
                BluePlaybackNet device = null;
				//WorkerThread.Singleton.PerformBlocking(() => {
					device = DeviceRegister.Singleton.GetDeviceHandle(deviceID);
				//});

				try
				{
					ModeRegister.Mode mode = null;
					//WorkerThread.Singleton.PerformBlocking(() =>
					//{
						//mode = ModeRegister.Singleton.Modes[modeString];
					//});

                    bool useCallback = false; // syncLoop != SyncLoop.Bluefish;
                    this.Source = new Source(device, BlueFish_h.EVideoMode.VID_FMT_1080P_6000, useCallback, FLogger);
					this.ReadTexture = new ReadTexture(1920, 1080, textureHandle, format, usage, FLogger);
					this.FBuffer = new byte[this.ReadTexture.BufferLength];

					if (useCallback)
					{
						//this.Source.NewFrame += Source_NewFrame;
					}
				}
				catch(Exception e)
				{
					if (this.Source != null)
						this.Source.Dispose();
					if (this.ReadTexture != null)
						this.ReadTexture.Dispose();
					if (this.FBuffer != null)
						this.FBuffer = null;
					throw e;
				}
			}

            

			void Source_NewFrame(IntPtr data)
			{
				lock (Source.LockBuffer)
				{
                    this.ReadTexture.ReadBack(data);
				}
			}

			public void PullFromTexture()
			{
                
                this.ReadTexture.ReadBack(Source);

			}

			public void UpdateFrameAvailable()
			{
				FFrameAvailable = FFrameWaiting;
				FFrameWaiting = false;
			}

			bool FFrameWaiting = false;
			bool FFrameAvailable = false;
			public bool FrameAvailable
			{
				get
				{
					return FFrameAvailable;
				}
			}

			public int FramesInBuffer
			{
				get
				{
					return Source.FramesInBuffer;
				}
			}

			public void Dispose()
			{
				this.Source.Dispose();
				this.ReadTexture.Dispose();
				this.FBuffer = null;
			}
		}

        #region fields & pins
#pragma warning disable 0649
		[Input("Device")]
		IDiffSpread<DeviceRegister.DeviceIndex> FInDevice;

		[Input("Mode")]
		IDiffSpread<ModeRegister.ModeIndex> FInMode;

		[Input("Format", EnumName = "TextureFormat")]
		IDiffSpread<EnumEntry> FInFormat;

		[Input("Usage", EnumName = "TextureUsage")]
		IDiffSpread<EnumEntry> FInUsage;

		[Input("Handle")]
		IDiffSpread<uint> FInHandle;

		[Input("Sync")]
		IDiffSpread<SyncLoop> FInSyncLoop;

		[Input("Enabled")]
		IDiffSpread<bool> FInEnabled;

		[Output("Frames In Buffer")]
		ISpread<int> FOutFramesInBuffer;

		[Output("Status")]
		ISpread<string> FOutStatus;

        [Import]
        ILogger FLogger;

		[Import]
		IHDEHost FHDEHost;

#pragma warning restore

        //track the current texture slice
		Spread<Instance> FInstances = new Spread<Instance>();
        #endregion fields & pins

        [ImportingConstructor]
		public VideoOut(IPluginHost host)
        {
        }

        public void Evaluate(int SpreadMax)
        {
			if (FInDevice.IsChanged || FInMode.IsChanged || FInFormat.IsChanged || FInUsage.IsChanged || FInHandle.IsChanged || FInEnabled.IsChanged || FInSyncLoop.IsChanged || FInSyncLoop.IsChanged)
			{

                DeviceRegister.SetupSingleton(FLogger);
				foreach(var slice in FInstances)
					if (slice != null)
						slice.Dispose();

				FInstances.SliceCount = 0;
				FOutStatus.SliceCount = SpreadMax;

				for (int i=0; i<SpreadMax; i++)
				{
					try
					{
                        /*
						if (FInDevice[i] == null)
							throw (new Exception("No device selected"));
						if (FInMode[i] == null)
							throw (new Exception("No mode selected"));
						if (FInEnabled[i] == false)
							throw (new Exception("Disabled"));
                        */

                        Instance inst = new Instance(0, "", FInHandle[i], FInFormat[i], FInUsage[i], FInSyncLoop[i], FLogger); 

						FInstances.Add(inst);
						FOutStatus[i] = "OK";
					}
					catch(Exception e)
					{
						FInstances.Add(null);
						FOutStatus[i] = e.ToString();
					}
				}
			}

			FOutFramesInBuffer.SliceCount = FInstances.SliceCount;
			for (int i = 0; i < FInstances.SliceCount; i++)
			{
				if (FInstances[i] == null)
					FOutFramesInBuffer[i] = 0;
				else
					FOutFramesInBuffer[i] = FInstances[i].FramesInBuffer;
			}
        }

		public void OnImportsSatisfied()
		{
			FHDEHost.MainLoop.OnPresent += MainLoop_Present;
			FHDEHost.MainLoop.OnRender += MainLoop_OnRender;
		}

		void MainLoop_Present(object o, EventArgs e)
		{
			/*TimeSpan sleepTime = new TimeSpan(100);
			Stopwatch waitingTime = new Stopwatch();
			waitingTime.Start();
			for (int i = 0; i < FInstances.SliceCount; i++)
			{
				if (FInSyncLoop[i] == SyncLoop.Sync && FInstances[i] != null)
				{
					var instance = FInstances[i];
					instance.UpdateFrameAvailable();
					while (!instance.FrameAvailable)
					{
						Thread.Sleep(sleepTime);
						instance.UpdateFrameAvailable();
						if (waitingTime.ElapsedMilliseconds > 100)
							break;
					}
				}
			}*/
		}

		void MainLoop_OnRender(object sender, EventArgs e)
		{
			for (int i = 0; i < FInstances.SliceCount; i++)
			{
				var instance = FInstances[i];
				if (instance == null)
					continue;

				/*if (FInSyncLoop[i] == SyncLoop.VVVV)
				{*/
					instance.PullFromTexture();
				//}
			}
		}

		public void Dispose()
		{
			foreach (var slice in FInstances)
				if (slice != null)
					slice.Dispose();
			GC.SuppressFinalize(this);

			FHDEHost.MainLoop.OnPresent -= MainLoop_Present;
			FHDEHost.MainLoop.OnRender -= MainLoop_OnRender;
		}

	}
}
