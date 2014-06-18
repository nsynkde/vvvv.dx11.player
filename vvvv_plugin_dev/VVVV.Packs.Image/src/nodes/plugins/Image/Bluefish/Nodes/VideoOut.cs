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

		class Instance : IDisposable
		{
			public Source Source;
			public ReadTexture ReadTexture;
            private uint FTextureWidth;
            private uint FTextureHeight;
            private uint FTextureHandle;
            private EnumEntry FTextureFormat;
            private EnumEntry FTextureUsage;
            private uint FDeviceID;

            ILogger FLogger;

            public Instance(uint deviceID, BlueFish_h.EVideoMode videoMode, uint width, uint height, uint textureHandle, EnumEntry format, EnumEntry usage, bool syncLoop, ILogger FLogger)
			{
                this.FLogger = FLogger;
                BluePlaybackNet device = DeviceRegister.Singleton.GetDeviceHandle((int)deviceID);
                FDeviceID = deviceID;

				try
				{
                    bool useCallback = false; // syncLoop != SyncLoop.Bluefish;
                    this.Source = new Source(device, videoMode, useCallback, FLogger);
                    FLogger.Log(LogType.Message, "Device video mode: " + Source.Width + "x" + Source.Height);
                    this.ReadTexture = new ReadTexture((int)width, (int)height, Source.Width, Source.Height, textureHandle, format, usage, FLogger);
                    this.SyncLoop = syncLoop;
                    this.FTextureHandle = textureHandle;
                    this.FTextureFormat = format;
                    this.FTextureUsage = usage;
                    this.FTextureWidth = width;
                    this.FTextureHeight = height;

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
					throw e;
				}
			}

            

			/*void Source_NewFrame(IntPtr data)
			{
				lock (Source.LockBuffer)
				{
                    this.ReadTexture.ReadBack(data);
				}
			}*/

			public void PullFromTexture()
			{
                
                this.ReadTexture.ReadBack(Source);

			}

			public void Dispose()
			{
				this.Source.Dispose();
				this.ReadTexture.Dispose();
			}

            public bool SyncLoop
            {
                set
                {
                    this.ReadTexture.FVSync = value;
                }

                get
                {
                    return this.ReadTexture.FVSync;
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
                    if (this.Source.VideoMode != value)
                    {
                        this.Source.VideoMode = value;
                        FLogger.Log(LogType.Message, "Device video mode: " + Source.Width + "x" + Source.Height);
                        bool sync = this.SyncLoop;
                        this.ReadTexture = new ReadTexture((int)FTextureWidth, (int)FTextureHeight, Source.Width, Source.Height, FTextureHandle, FTextureFormat, FTextureUsage, FLogger);
                        this.ReadTexture.FVSync = sync;
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
                    return DeviceRegister.Singleton.GetSerialNumber((int)FDeviceID);
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

        [Input("TextureWidth")]
        IDiffSpread<uint> FInWidth;

        [Input("TextureHeight")]
        IDiffSpread<uint> FInHeight;

        [Input("TextureFormat", EnumName = "TextureFormat")]
        IDiffSpread<EnumEntry> FInFormat;

        [Input("TextureUsage", EnumName = "TextureUsage")]
        IDiffSpread<EnumEntry> FInUsage;

        [Input("TextureHandle")]
        IDiffSpread<uint> FInHandle;

		[Output("Status")]
        ISpread<string> FOutStatus;

        [Output("Serial Number")]
        ISpread<string> FOutSerialNumber;

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
			if (FInDevice.IsChanged || FInWidth.IsChanged || FInHeight.IsChanged || FInFormat.IsChanged || FInUsage.IsChanged || FInHandle.IsChanged || FInEnabled.IsChanged )
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
						if (FInEnabled[i] == false)
							throw (new Exception("Disabled"));

                        Instance inst = new Instance(FInDevice[i], FInMode[i], FInWidth[i], FInHeight[i], FInHandle[i], FInFormat[i], FInUsage[i], true, FLogger); 

						FInstances.Add(inst);
						FOutStatus[i] = "OK";
                        FOutSerialNumber[i] = inst.DeviceSerialNumber;
					}
					catch(Exception e)
					{
						FInstances.Add(null);
						FOutStatus[i] = e.ToString();
					}
				}
			}

            if (FInMode.IsChanged)
            {
                for (int i = 0; i < FInstances.SliceCount; i++)
                {
                    if (FInstances[i] != null)
                        FInstances[i].VideoMode = FInMode[i];
                }

            }

            if (FInOutChannel.IsChanged)
            {
                for (int i = 0; i < FInstances.SliceCount; i++)
                {
                    if (FInstances[i] != null)
                        FInstances[i].Channel = FInOutChannel[i];
                }

            }

            if (FInOutputColorSpace.IsChanged)
            {
                for (int i = 0; i < FInstances.SliceCount; i++)
                {
                    if (FInstances[i] != null)
                        FInstances[i].OutputColorSpace = FInOutputColorSpace[i];
                }

            }

            if (FInSyncLoop.IsChanged)
            {
                for (int i = 0; i < FInstances.SliceCount; i++)
                {
                    if (FInstances[i] != null)
                        FInstances[i].SyncLoop = FInSyncLoop[i];
                }

            }

            if (FInDualLink.IsChanged)
            {
                for (int i = 0; i < FInstances.SliceCount; i++)
                {
                    if (FInstances[i] != null)
                        FInstances[i].DualLink = FInDualLink[i];
                }

            }

            if (FInDualLinkSignalFormat.IsChanged)
            {
                for (int i = 0; i < FInstances.SliceCount; i++)
                {
                    if (FInstances[i] != null)
                        FInstances[i].DualLinkSignalFormat = FInDualLinkSignalFormat[i];
                }

            }

            if (FInColorMatrix.IsChanged)
            {
                for (int i = 0; i < FInstances.SliceCount; i++)
                {
                    if (FInstances[i] != null)
                        FInstances[i].ColorMatrix = FInColorMatrix[i];
                }

            }
        }

		public void OnImportsSatisfied()
		{
			FHDEHost.MainLoop.OnPresent += MainLoop_Present;
		}

		void MainLoop_Present(object o, EventArgs e)
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
		}

	}
}
