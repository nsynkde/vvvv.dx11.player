#region usings
using System;
using System.ComponentModel.Composition;
using System.Runtime.InteropServices;

using VVVV.Core.Logging;
using VVVV.PluginInterfaces.V1;
using VVVV.PluginInterfaces.V2;
using VVVV.PluginInterfaces.V2.EX9;
using VVVV.Utils.VColor;
using VVVV.Utils.VMath;
using VVVV.Utils.SlimDX;

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

		class Instance : IDisposable
		{
			public BluefishSource Source;
			public ReadTextureDX11 ReadTexture;
            private ulong FTextureHandle;
            private uint FDeviceID;
            private uint FDroppedFrames = 0;
            private WorkerThread FUploadThread = new WorkerThread();
            private WorkerThread FRenderThread = new WorkerThread();
            private Stopwatch FLogTimer = new Stopwatch();
            double FAccTime;
            double FAvgTime;
            double FMaxTime;
            uint FNumFrames;

            ILogger FLogger;

            public Instance(uint deviceID, uint channel, BlueFish_h.EVideoMode videoMode, BlueFish_h.EMemoryFormat outFormat, ulong textureHandle, bool syncLoop, ILogger FLogger)
			{
                this.FLogger = FLogger;
                FDeviceID = deviceID;

				try
				{
                    this.ReadTexture = new ReadTextureDX11(textureHandle, outFormat, FLogger);
                    this.Source = new BluefishSource(deviceID, channel, videoMode, outFormat, FLogger);
                    if (Source.Width != ReadTexture.InputTexWidth || Source.Height != ReadTexture.InputTexHeight)
                    {
                        throw new Exception("Input texture and video format have different sizes");
                    }

                    FLogger.Log(LogType.Message, "Device video mode: " + Source.Width + "x" + Source.Height);
                    this.SyncLoop = syncLoop;
                    this.FTextureHandle = textureHandle;
                    this.FLogTimer.Start();
				}
				catch(Exception e)
				{
					if (this.Source != null)
						this.Source.Dispose();
					if (this.ReadTexture != null)
						this.ReadTexture.Dispose();
					throw e;
				}
			}

			public void PullFromTexture()
            {
                if (FRenderThread.QueueSize < 4)
                {
                    FRenderThread.Perform(() =>
                    {
                        Stopwatch timer = new Stopwatch();
                        timer.Start();
                        Source.RenderNext();
                        timer.Stop();
                        LogTime(timer.Elapsed.TotalMilliseconds,true);
                    });

                }
                if (FUploadThread.QueueSize<4)
                {
                    FUploadThread.Perform(() =>
                    {
                        Stopwatch timer = new Stopwatch();
                        timer.Start();
                        var memory = this.ReadTexture.ReadBack();
                        if (memory != (IntPtr)0)
                        {
                            Source.SendFrame(memory);
                        }
                        else
                        {
                            FDroppedFrames++;
                        }
                        timer.Stop();
                        LogTime(timer.Elapsed.TotalMilliseconds,false);
                    });
                }
                else
                {
                    FDroppedFrames++;
                }
                if (this.SyncLoop)
                {
                    Source.WaitSync();
                }
			}

            private void LogTime(double lastTime, bool renderTime)
            {
                /*if (lastTime > 15)
                {
                    FLogger.Log(LogType.Error, "Error!!!!  Total Time: " + lastTime.ToString());
                }*/
                FAccTime += lastTime;
                if(!renderTime) FNumFrames++;
                if (lastTime > FMaxTime)
                {
                    FMaxTime = lastTime;
                }
                if (FLogTimer.ElapsedMilliseconds >= 2000)
                {
                    FLogTimer.Reset();
                    FAccTime /= FNumFrames;
                    //FLogger.Log(LogType.Message, "Total Avg Time: " + FAvgTime);
                    //FLogger.Log(LogType.Message, "Total Max Time: " + FMaxTime);
                    FAvgTime = FAccTime;
                    FAccTime = 0;
                    FNumFrames = 0;
                    FMaxTime = 0;
                    FLogTimer.Start();
                }
            }

			public void Dispose()
			{
				this.Source.Dispose();
				this.ReadTexture.Dispose();
			}

            bool FSyncLoop;
            public bool SyncLoop
            {
                set
                {
                    this.FSyncLoop = value;
                }

                get
                {
                    return this.FSyncLoop;
                }
            }

            public bool DualLink
            {
                set
                {
                    this.Source.DualLink = value;
                }

                get
                {
                    return this.Source.DualLink;
                }
            }

            public BlueFish_h.EDualLinkSignalFormatType DualLinkSignalFormat
            {
                set
                {
                    this.Source.DualLinkSignalFormat = value;
                }

                get
                {
                    return this.Source.DualLinkSignalFormat;
                }
            }

            public BlueFish_h.EPreDefinedColorSpaceMatrix ColorMatrix
            {
                set
                {
                    this.Source.ColorMatrix = value;
                }

                get
                {
                    return this.Source.ColorMatrix;
                }
            }

            public BlueFish_h.EConnectorSignalColorSpace OutputColorSpace
            {
                set
                {
                    this.Source.OutputColorSpace = value;
                }

                get
                {
                    return this.Source.OutputColorSpace;
                }
            }

            public BlueFish_h.EVideoMode VideoMode
            {
                set
                {
                    this.Source.VideoMode = value;
                    if (Source.Width != ReadTexture.InputTexWidth || Source.Height != ReadTexture.InputTexHeight)
                    {
                        throw new Exception("Input texture and video format have different sizes");
                    }
                }

                get
                {
                    return this.Source.VideoMode;
                }
            }

            public uint Channel
            {
                set
                {
                    this.Source.Channel = value;
                }

                get
                {
                    return this.Source.Channel;
                }
            }

            public string DeviceSerialNumber
            {
                get
                {
                    return this.Source.SerialNumber;
                }
            }

            public uint DroppedFrames
            {
                get
                {
                    return FDroppedFrames;
                }
            }

            public double AvgFrameTime
            {
                get
                {
                    return FAvgTime;
                }
            }

            public double MaxFrameTime
            {
                get
                {
                    return FMaxTime;
                }
            }
		}

        #region fields & pins
#pragma warning disable 0649
		[Input("Device")]
        IDiffSpread<uint> FInDevice;

        [Input("Out Channel")]
        IDiffSpread<uint> FInOutChannel;

        [Input("Mode")]
        IDiffSpread<BlueFish_h.EVideoMode> FInMode;

        [Input("Format")]
        IDiffSpread<BlueFish_h.EMemoryFormat> FInFormat;

        [Input("Out Color Space")]
        IDiffSpread<BlueFish_h.EConnectorSignalColorSpace> FInOutputColorSpace;

        [Input("Dual Link")]
        IDiffSpread<bool> FInDualLink;

        [Input("Dual Link Signal Format")]
        IDiffSpread<BlueFish_h.EDualLinkSignalFormatType> FInDualLinkSignalFormat;

        [Input("Color Matrix")]
        IDiffSpread<BlueFish_h.EPreDefinedColorSpaceMatrix> FInColorMatrix;

		[Input("Sync")]
		IDiffSpread<bool> FInSyncLoop;

		[Input("Enabled")]
        IDiffSpread<bool> FInEnabled;

        [Input("TextureHandle")]
        IDiffSpread<ulong> FInHandle;

		[Output("Status")]
        ISpread<string> FOutStatus;

        [Output("Serial Number")]
        ISpread<string> FOutSerialNumber;

        [Output("Dropped Frames")]
        ISpread<uint> FOutDroppedFrames;

        [Output("Avg. Frame Time")]
        ISpread<double> FOutAvgFrameTime;

        [Output("Max. Frame Time")]
        ISpread<double> FOutMaxFrameTime;

        [Output("Out Channel")]
        ISpread<uint> FOutOutChannel;

        [Import]
        ILogger FLogger;

		[Import]
		IHDEHost FHDEHost;

#pragma warning restore

        //track the current texture slice
		Spread<Instance> FInstances = new Spread<Instance>();
        bool FFirstRun = true;
        #endregion fields & pins

        [DllImport("kernel32.dll", SetLastError = false)]
        internal static extern int AddDllDirectory(string directory);

        /*static VideoOut()
        {
            try{
                var type = System.Type.;
                var codebase = System.Reflection.Assembly.GetAssembly(type).GetName().CodeBase;
                var pluginfolder = System.IO.Path.GetDirectoryName(codebase);
                AddDllDirectory(pluginfolder);
            }catch(Exception e){
            }

        }*/

        [ImportingConstructor]
		public VideoOut(IPluginHost host)
        {
        }

        public void Evaluate(int SpreadMax)
        {
            if (FFirstRun || FInDevice.IsChanged || FInMode.IsChanged || FInFormat.IsChanged || FInHandle.IsChanged || FInEnabled.IsChanged || FInOutChannel.IsChanged)
			{
				foreach(var slice in FInstances)
					if (slice != null)
						slice.Dispose();

				FInstances.SliceCount = 0;
				FOutStatus.SliceCount = SpreadMax;
                FOutAvgFrameTime.SliceCount = SpreadMax;
                FOutDroppedFrames.SliceCount = SpreadMax;
                FOutMaxFrameTime.SliceCount = SpreadMax;
                FOutOutChannel.SliceCount = SpreadMax;

				for (int i=0; i<SpreadMax; i++)
				{
					try
					{
						if (FInEnabled[i] == false)
							throw (new Exception("Disabled"));

                        Instance inst = new Instance(FInDevice[i], FInOutChannel[i], FInMode[i], FInFormat[i], FInHandle[i], true, FLogger); 

						FInstances.Add(inst);
						FOutStatus[i] = "OK";
                        FOutSerialNumber[i] = inst.DeviceSerialNumber;
                        FOutOutChannel[i] = FInOutChannel[i];
					}
					catch(Exception e)
					{
						FInstances.Add(null);
						FOutStatus[i] = e.ToString();
					}
				}

                FFirstRun = true;
			}

            if (FFirstRun || FInOutputColorSpace.IsChanged)
            {
                for (int i = 0; i < FInstances.SliceCount; i++)
                {
                    if (FInEnabled[i] != false && FInstances[i] != null)
                        FInstances[i].OutputColorSpace = FInOutputColorSpace[i];
                }

            }

            if (FFirstRun || FInSyncLoop.IsChanged)
            {
                for (int i = 0; i < FInstances.SliceCount; i++)
                {
                    if (FInEnabled[i] != false && FInstances[i] != null)
                        FInstances[i].SyncLoop = FInSyncLoop[i];
                }

            }

            if (FFirstRun || FInDualLinkSignalFormat.IsChanged)
            {
                for (int i = 0; i < FInstances.SliceCount; i++)
                {
                    if (FInEnabled[i] != false && FInstances[i] != null)
                        FInstances[i].DualLinkSignalFormat = FInDualLinkSignalFormat[i];
                }

            }

            if (FFirstRun || FInDualLink.IsChanged)
            {
                for (int i = 0; i < FInstances.SliceCount; i++)
                {
                    if (FInEnabled[i] != false && FInstances[i] != null)
                        FInstances[i].DualLink = FInDualLink[i];
                }

            }

            if (FFirstRun || FInColorMatrix.IsChanged)
            {
                for (int i = 0; i < FInstances.SliceCount; i++)
                {
                    if (FInEnabled[i] != false && FInstances[i] != null)
                        FInstances[i].ColorMatrix = FInColorMatrix[i];
                }

            }

            FFirstRun = false;
            for (int i = 0; i < FInstances.SliceCount; i++)
            {
                if (FInEnabled[i] != false && FInstances[i] != null){
                    FOutDroppedFrames[i] = FInstances[i].DroppedFrames;
                    FOutAvgFrameTime[i] = FInstances[i].AvgFrameTime;
                    FOutMaxFrameTime[i] = FInstances[i].MaxFrameTime;
                }
            }
        }

		public void OnImportsSatisfied()
        {
            //FHDEHost.MainLoop.OnRender += MainLoop_Present;
			FHDEHost.MainLoop.OnPresent += MainLoop_Present;
		}

		void MainLoop_Present(object o, EventArgs e)
		{
            for (int i = 0; i < FInstances.SliceCount; i++)
            {
                var instance = FInstances[i];
                if (instance == null)
                    continue;

                instance.PullFromTexture();
            }
		}

		public void Dispose()
		{
			foreach (var slice in FInstances)
				if (slice != null)
					slice.Dispose();
			GC.SuppressFinalize(this);

            //FHDEHost.MainLoop.OnRender -= MainLoop_Present;
			FHDEHost.MainLoop.OnPresent -= MainLoop_Present;
		}

	}
}
