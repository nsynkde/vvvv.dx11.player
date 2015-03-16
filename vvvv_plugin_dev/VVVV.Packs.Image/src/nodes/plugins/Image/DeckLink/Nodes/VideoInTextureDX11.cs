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

#endregion usings

using DeckLinkAPI;
using System.Collections.Generic;
using System.Diagnostics;
using VVVV.DX11;
using FeralTic.DX11.Resources;
using SlimDX.Direct3D11;
using System.Collections.Concurrent;
using System.Threading;

namespace VVVV.Nodes.DeckLink
{
	#region PluginInfo
	[PluginInfo(Name = "VideoIn",
				Category = "DeckLink",
				Version = "DX11.Texture",
				Help = "Capture a video stream to a texture",
				Author = "arturo",
				Credits = "Nsynk",
				Tags = "")]
	#endregion PluginInfo
    public class VideoInTextureDX11 : IPluginEvaluate, IDisposable, IDX11ResourceProvider
	{
		#region fields & pins

		[Input("Device")]
		IDiffSpread<DeviceRegister.DeviceIndex> FPinInDevice;

		[Input("Video mode")]
		IDiffSpread<_BMDDisplayMode> FPinInMode;

		[Input("Flags")]
        IDiffSpread<_BMDVideoInputFlags> FPinInFlags;

        [Input("Sync")]
        IDiffSpread<bool> FSync;

        [Input("Enabled", DefaultValue=1)]
        IDiffSpread<bool> FEnabled;

		[Input("Flush Streams", IsBang = true)]
		ISpread<bool> FPinInFlush;

		[Output("Frames Available")]
		ISpread<int> FPinOutFramesAvailable;

		[Output("Frame Received", IsBang=true)]
        ISpread<bool> FPinOutFrameReceived;

		[Output("Status")]
		ISpread<string> FStatus;

        [Output("TextureDX11")]
        ISpread<DX11Resource<DX11Texture2D>> FOutTexture;


        ISpread<Texture2D[]> FStagingTexture = new Spread<Texture2D[]>();
        ISpread<DataBox[]> FStagingDataBox = new Spread<DataBox[]>();
        ISpread<BlockingCollection<int>> FReadyToUpload = new Spread<BlockingCollection<int>>();
        ISpread<BlockingCollection<int>> FReadyToRender = new Spread<BlockingCollection<int>>();
        Mutex mutex = new Mutex();

		List<Capture> FCaptures = new List<Capture>();
		#endregion fields & pins

        private bool FRefreshTextures;
        private bool FFirstUse = true;

        [Import]
        ILogger FLogger;


		// import host and hand it to base constructor
		[ImportingConstructor()]
        public VideoInTextureDX11(IPluginHost host)
		{
		}

		//called when data for any output pin is requested
		public void Evaluate(int SpreadMax)
		{
			FStatus.SliceCount = SpreadMax;
			FPinOutFramesAvailable.SliceCount = SpreadMax;
			FPinOutFrameReceived.SliceCount = SpreadMax;
            FOutTexture.SliceCount = SpreadMax;
            FStagingTexture.SliceCount = SpreadMax;
            FStagingDataBox.SliceCount = SpreadMax;
            FReadyToUpload.SliceCount = SpreadMax;
            FReadyToRender.SliceCount = SpreadMax;

			while (FCaptures.Count < SpreadMax)
            {
                var capture = new Capture();
                capture.DeviceIdx = FCaptures.Count;
                FCaptures.Add(capture);
			}
			while (FCaptures.Count > SpreadMax)
			{
				FCaptures[FCaptures.Count - 1].Dispose();
				FCaptures.RemoveAt(FCaptures.Count - 1);
			}

            if (FEnabled.IsChanged)
            {
                for (int i = 0; i < FEnabled.SliceCount; i++)
                {
                    if (!FEnabled[i] && FCaptures[i] != null)
                    {
                        FCaptures[i].Close();
                    }
                    else if (FEnabled[i])
                    {
                        FRefreshTextures = true;
                    }
                }
            }

            if (FPinInMode.IsChanged || FPinInDevice.IsChanged || FPinInFlags.IsChanged || FEnabled.IsChanged)
			{
                mutex.WaitOne();
                for (int i = 0; i < SpreadMax; i++)
                {
                    if (FEnabled[i])
                    {
                        ReOpen(i);
                    }
                }
                FRefreshTextures = true;
                mutex.ReleaseMutex();
			}

			bool reinitialise = false;
			foreach (var capture in FCaptures)
			{
				reinitialise |= capture.Reinitialise;
			}

			for (int i = 0; i < SpreadMax; i++)
			{
                if (FEnabled[i])
                {
                    FPinOutFramesAvailable[i] = FCaptures[i].AvailableFrameCount;
                }
                else
                {
                    FPinOutFramesAvailable[i] = 0;
                }
			}

            if (FFirstUse)
            {
                FRefreshTextures = true;
                FFirstUse = false;
            }
		}

		void ReOpen(int index)
		{
			try
			{
				FCaptures[index].Open(FPinInDevice[index], FPinInMode[index], FPinInFlags[index]);
				FStatus[index] = "OK";
			}

			catch (Exception e)
			{
				FStatus[index] = e.Message;
			}
		}

        private void NewFrame(IntPtr Data, int DeviceIdx)
        {
            int nextFrame;
            bool newFrame = FReadyToUpload[DeviceIdx].TryTake(out nextFrame,100);
            if (newFrame)
            {
                mutex.WaitOne();
                var data = FStagingDataBox[DeviceIdx][nextFrame];
                data.Data.WriteRange(FCaptures[DeviceIdx].Data, FCaptures[DeviceIdx].BytesPerFrame);
                mutex.ReleaseMutex();
                FReadyToRender[DeviceIdx].Add(nextFrame);
            }
        }

		/*DateTime FLastUpdate = DateTime.Now;
        protected void UpdateTexture(int Slice, Texture2D stagingTexture, Texture2D outTexture, DeviceContext context)
		{
			FPinOutFrameReceived[Slice] = false;

			if (!FCaptures[Slice].Ready)
				return;

            double timeout = FSync[Slice]?30:0;
            if (timeout == 0)
            {
                if (!FCaptures[Slice].FreshData)
                    return;
            }
            else
            {
                while (!FCaptures[Slice].FreshData)
                {
                    if ((DateTime.Now - FLastUpdate).TotalMilliseconds > timeout)
                    {
                        return; //timeout occured
                    }
                    else
                    {
                        System.Threading.Thread.Sleep(1);
                    }
                }
                FLastUpdate = DateTime.Now;
            }

            var data = context.MapSubresource(stagingTexture, 0, MapMode.Write, MapFlags.None);

			try
			{
				FCaptures[Slice].Lock.WaitOne();
				try
				{
                    data.Data.WriteRange(FCaptures[Slice].Data, FCaptures[Slice].BytesPerFrame);
					FCaptures[Slice].Updated();
					FPinOutFrameReceived[Slice] = true;
				}
				catch
				{
					FStatus[Slice] = "Failure to upload texture.";
				}
				finally
				{
                    context.UnmapSubresource(stagingTexture, 0);
                    FCaptures[Slice].Lock.ReleaseMutex();
                    context.CopyResource(stagingTexture, outTexture);
				}
			}
			catch
			{
				FStatus[Slice] = "Failure to lock data for reading.";
			}
		}*/

		public void Dispose()
        {
            FLogger.Log(LogType.Message, "Dispose");
            mutex.WaitOne();
            foreach (var capture in FCaptures)
                capture.FFrameCallback = null;
            mutex.ReleaseMutex();
			foreach (var capture in FCaptures)
				capture.Dispose();
			GC.SuppressFinalize(this);
		}

        public void Destroy(IPluginIO pin, FeralTic.DX11.DX11RenderContext context, bool force)
        {
            FLogger.Log(LogType.Message, "DESTROY");
            mutex.WaitOne();
            foreach (var capture in FCaptures)
                capture.FFrameCallback = null;
            FRefreshTextures = true;
            mutex.ReleaseMutex();
            /*Dispose();
            mutex.WaitOne();
            foreach (var texBuffer in FStagingTexture)
            {
                foreach (var tex in texBuffer)
                {
                    tex.Dispose();
                }
            }
            foreach (var tex in FOutTexture)
            {
                tex[context].Dispose();
            }
            mutex.ReleaseMutex();*/
        }

        public void Update(IPluginIO pin, FeralTic.DX11.DX11RenderContext context)
        {
            if (FRefreshTextures)
            {
                mutex.WaitOne();
                for(int i=0;i<FOutTexture.SliceCount;i++){

                    Texture2DDescription desc = new Texture2DDescription();
                    desc.ArraySize = 1;
                    desc.BindFlags = BindFlags.None;
                    desc.CpuAccessFlags = CpuAccessFlags.Write;
                    desc.Format = SlimDX.DXGI.Format.R8G8B8A8_UNorm;
                    desc.Height = FCaptures[i].Height;
                    desc.Width = FCaptures[i].Width / 2;
                    desc.MipLevels = 1;
                    desc.OptionFlags = ResourceOptionFlags.None;
                    desc.SampleDescription = new SlimDX.DXGI.SampleDescription(1, 0);
                    desc.Usage = ResourceUsage.Staging;
                    if (FEnabled[i])
                    {
                        FStagingTexture[i] = new Texture2D[2];
                        FStagingDataBox[i] = new DataBox[2];
                        FReadyToUpload[i] = new BlockingCollection<int>(new ConcurrentQueue<int>());
                        FReadyToRender[i] = new BlockingCollection<int>(new ConcurrentQueue<int>());
                        FLogger.Log(LogType.Message, "Creating staging textures");
                        for (int j = 0; j < FStagingTexture[i].Length; j++)
                        {
                            FStagingTexture[i][j] = new Texture2D(context.Device, desc);
                            FStagingDataBox[i][j] = context.Device.ImmediateContext.MapSubresource(FStagingTexture[i][j], 0, MapMode.Write, MapFlags.None);
                            FReadyToUpload[i].Add(j);
                        }

                        FCaptures[i].FFrameCallback = new Capture.FrameCallbackDelegate(NewFrame);
                    }
                    desc.Usage = ResourceUsage.Default;
                    desc.CpuAccessFlags = CpuAccessFlags.None;
                    desc.BindFlags = BindFlags.ShaderResource;
                    var tex = DX11Texture2D.FromDescription(context, desc);
                    FOutTexture[i] = new DX11Resource<DX11Texture2D>();
                    FOutTexture[i][context] = tex;
                }
                mutex.ReleaseMutex();
                FRefreshTextures = false;
            }
            for (int i = 0; i < FOutTexture.SliceCount; i++)
            {
                //UpdateTexture(i, FStagingTexture[i][0], FOutTexture[i][context].Resource, context.Device.ImmediateContext);

                if (FEnabled[i])
                {
                    int nextFrame;
                    bool newFrame;
                    if (FSync[i])
                    {
                        newFrame = true;
                        nextFrame = FReadyToRender[i].Take();
                    }
                    else
                    {
                        newFrame = FReadyToRender[i].TryTake(out nextFrame);
                    }
                    if (newFrame)
                    {
                        context.Device.ImmediateContext.UnmapSubresource(FStagingTexture[i][nextFrame], 0);
                        context.Device.ImmediateContext.CopyResource(FStagingTexture[i][nextFrame], FOutTexture[i][context].Resource);
                        FStagingDataBox[i][nextFrame] = context.Device.ImmediateContext.MapSubresource(FStagingTexture[i][nextFrame], 0, MapMode.Write, MapFlags.None);
                        FReadyToUpload[i].Add(nextFrame);
                    }
                    FPinOutFrameReceived[i] = newFrame;
                }
                else
                {
                    FPinOutFrameReceived[i] = false;
                }
            }
        }
    }
}
