#region usings
using System;
using System.ComponentModel.Composition;
using System.Runtime.InteropServices;

using System.Threading;
using System.Diagnostics;

using VVVV.Core.Logging;
using VVVV.PluginInterfaces.V1;
using VVVV.PluginInterfaces.V2;
using System.IO;
using System.Reflection;
using FeralTic.DX11.Queries;
using FeralTic.DX11;
using FeralTic.DX11.Resources;
using VVVV.DX11;
using SlimDX.Direct3D11;
using VVVV.Nodes.Bluefish.src.Helpers;

#endregion usings

namespace VVVV.Nodes.Bluefish
{
    
    #region PluginInfo
    [PluginInfo(Name = "VideoOutDX11",
                Category = "Bluefish",
                Help = "Given a texture handle, will push graphic to Bluefish device",
                Tags = "",
                Author = "nsynk",
                AutoEvaluate = true)]
    #endregion PluginInfo
    public class VideoOutDX11 : IPluginEvaluate, IDisposable, IDX11RendererProvider, IDX11RenderWindow
    {

        #region fields & pins
#pragma warning disable 0649
		[Input("Device")]
        IDiffSpread<uint> FInDevice;

        [Input("Out Channel")]
        IDiffSpread<uint> FInOutChannel;

        [Input("Mode", DefaultValue = (int)BlueFish_h.EVideoMode.VID_FMT_1080P_6000)]
        IDiffSpread<BlueFish_h.EVideoMode> FInMode;

        [Input("Format", DefaultValue = (int)BlueFish_h.EMemoryFormat.MEM_FMT_YUVS)]
        IDiffSpread<BlueFish_h.EMemoryFormat> FInFormat;

        [Input("Out Color Space", DefaultValue = (int)BlueFish_h.EConnectorSignalColorSpace.YUV_ON_CONNECTOR)]
        IDiffSpread<BlueFish_h.EConnectorSignalColorSpace> FInOutputColorSpace;

        [Input("Dual Link")]
        IDiffSpread<bool> FInDualLink;

        [Input("Dual Link Signal Format")]
        IDiffSpread<BlueFish_h.EDualLinkSignalFormatType> FInDualLinkSignalFormat;

        [Input("Color Matrix")]
        IDiffSpread<BlueFish_h.EPreDefinedColorSpaceMatrix> FInColorMatrix;

        [Input("Num. Render Target Buffers", DefaultValue = 2, MinValue = 1)]
        IDiffSpread<int> FNumRenderTargetsBuffers;

        [Input("Num. Read Back Buffers", DefaultValue = 2, MinValue = 1)]
        IDiffSpread<int> FNumReadBackBuffers;

        [Input("Num. Bluefish Buffers", DefaultValue = 4, MinValue = 1)]
        IDiffSpread<int> FNumBluefishBuffers;

		[Input("Sync", DefaultValue = 1)]
		IDiffSpread<bool> FInSyncLoop;

		[Input("Enabled")]
        IDiffSpread<bool> FInEnabled;

        [Input("TextureHandleDX11")]
        IDiffSpread<ulong> FInHandleDX11;

        [Input("TextureDX11")]
        ISpread<DX11Resource<DX11Texture2D>> FInTexture;

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
        Spread<BluefishSource> FInstances = new Spread<BluefishSource>();

        Spread<SharedTexBuffer> FSharedTextureBuffer = new Spread<SharedTexBuffer>();

        bool FFirstRun = true;
        #endregion fields & pins
        bool DX11ChangedLast, DirectTextureChanged;
        private DX11RenderContext primary;

        [DllImport("kernel32.dll", CharSet = CharSet.Auto)]
        private static extern void OutputDebugString(string message);

        [DllImport("kernel32.dll", CharSet = CharSet.Auto)]
        private static extern IntPtr AddDllDirectory(string newdirectory);

        [DllImport("kernel32.dll")]
        private static extern bool SetDefaultDllDirectories(UInt32 DirectoryFlags);

        const UInt32 LOAD_LIBRARY_SEARCH_APPLICATION_DIR = 0x00000200;
        const UInt32 LOAD_LIBRARY_SEARCH_USER_DIRS = 0x00000400;
        const UInt32 LOAD_LIBRARY_SEARCH_DEFAULT_DIRS = 0x00001000;
        const UInt32 LOAD_LIBRARY_SEARCH_SYSTEM32 = 0x00000800;
        
        static VideoOutDX11()
        {
            try
            {
                string codeBase = Assembly.GetExecutingAssembly().CodeBase;
                UriBuilder uri = new UriBuilder(codeBase);
                string path = Uri.UnescapeDataString(uri.Path);
                string pluginfolder = Path.GetDirectoryName(path);
                SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_APPLICATION_DIR | LOAD_LIBRARY_SEARCH_USER_DIRS | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS | LOAD_LIBRARY_SEARCH_SYSTEM32);
                AddDllDirectory(pluginfolder);
                IntPtr check64 = (IntPtr)0;
                if (System.Runtime.InteropServices.Marshal.SizeOf(check64) == sizeof(Int64))
                {
                    pluginfolder = pluginfolder + "\\" + "x64\\";
                }
                else
                {
                    pluginfolder = pluginfolder + "\\" + "x86\\";
                }
                AddDllDirectory(pluginfolder);
            }catch(Exception){
            }

        }

        [ImportingConstructor]
        public VideoOutDX11(IPluginHost host)
        {
        }

        public void Evaluate(int SpreadMax)
        {
            if (FInTexture.IsChanged)
            {
                DirectTextureChanged = true;
                DX11ChangedLast = false;
            }else if(FInHandleDX11.IsChanged)
            {
                DirectTextureChanged = false;
                DX11ChangedLast = true;

            }

            if (FFirstRun || FInDevice.IsChanged || FInMode.IsChanged ||
                FInFormat.IsChanged || FInHandleDX11.IsChanged || FInTexture.IsChanged || 
                FInEnabled.IsChanged || FInOutChannel.IsChanged || 
                FNumRenderTargetsBuffers.IsChanged || FNumReadBackBuffers.IsChanged || FNumBluefishBuffers.IsChanged)
			{
				foreach(var slice in FInstances)
					if (slice != null)
						slice.Dispose();


				FInstances.SliceCount = 0;
                FSharedTextureBuffer.SliceCount = 0;
				FOutStatus.SliceCount = SpreadMax;
                FOutAvgFrameTime.SliceCount = SpreadMax;
                FOutDroppedFrames.SliceCount = SpreadMax;
                FOutMaxFrameTime.SliceCount = SpreadMax;
                FOutOutChannel.SliceCount = SpreadMax;

                FFirstRun = true;
			}
            for (int i = 0; i < FInstances.SliceCount; i++)
            {
                if (FInEnabled[i] != false && FInstances[i] != null){
                    //FOutDroppedFrames[i] = FInstances[i].DroppedFrames;
                    FOutAvgFrameTime[i] = FInstances[i].AvgDuration;
                    FOutMaxFrameTime[i] = FInstances[i].MaxDuration;
                }
            }
        }


        public void Update(IPluginIO pin, DX11RenderContext context)
        {

        }


        public void Render(DX11RenderContext context)
        {
            if (FFirstRun)
            {

                for (int i = 0; i < FInEnabled.SliceCount; i++)
                {
                    try
                    {
                        if (FInEnabled[i] == false)
                            throw (new Exception("Disabled"));

                        //var handle = (DX11ChangedLast && FInHandleDX11[i] != 0) ? FInHandleDX11[i] : FInHandleDX9[i];

                        /*Texture2D t = this.FInTexture[i][context].Resource;
                        Texture2DDescription desc = t.Description;
                        desc.BindFlags = BindFlags.ShaderResource;
                        desc.OptionFlags = ResourceOptionFlags.Shared;
                        desc.MipLevels = 1;
                        var tex = new Texture2D(context.Device, desc);
                        var SharedResource = new SlimDX.DXGI.Resource(tex);
                        var handle = (ulong)SharedResource.SharedHandle;

                        ulong handle = (ulong)this.FInTexture[i][context].Resource.ComPointer;*/

                        //var sharedTexBuffer = new SharedTexBuffer(this.FInTexture[i][context].Resource, context.Device, 4);

                        ulong handle = 0;
                        if (DirectTextureChanged)
                        {
                            var SharedResource = new SlimDX.DXGI.Resource(this.FInTexture[i][context].Resource);
                            handle = (ulong)SharedResource.SharedHandle;
                        }else if(DX11ChangedLast){
                            handle = FInHandleDX11[i];
                        }


                        BluefishSource inst = new BluefishSource(FInDevice[i], FInOutChannel[i], FInMode[i], FInFormat[i], handle/*sharedTexBuffer.FrontSharedHandle*/, FNumRenderTargetsBuffers[i], FNumReadBackBuffers[i], FNumBluefishBuffers[i], FLogger);

                        //FSharedTexture.Add(tex);
                        //FSharedTextureBuffer.Add(sharedTexBuffer);
                        FInstances.Add(inst);
                        FOutStatus[i] = "OK";
                        FOutSerialNumber[i] = inst.SerialNumber;
                        FOutOutChannel[i] = FInOutChannel[i];
                    }
                    catch (Exception e)
                    {
                        FInstances.Add(null);
                        FOutStatus[i] = e.ToString();
                    }
                }
            }

            if (FFirstRun || FInOutputColorSpace.IsChanged)
            {
                for (int i = 0; i < FInstances.SliceCount; i++)
                {
                    if (FInEnabled[i] != false && FInstances[i] != null)
                        FInstances[i].OutputColorSpace = FInOutputColorSpace[i];
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

            /*for (int i = 0; i < FInstances.SliceCount; i++)
            {
                //context.Device.ImmediateContext.CopyResource(this.FInTexture[i][context].Resource, FSharedTexture[i]);
                FSharedTextureBuffer[i].OnRender();
                FInstances[i].SharedHandle = FSharedTextureBuffer[i].FrontSharedHandle;
            }*/
        }

		public void Dispose()
		{
			foreach (var slice in FInstances)
				if (slice != null)
					slice.Dispose();
			GC.SuppressFinalize(this);
		}

        public void Destroy(IPluginIO pin, DX11RenderContext context, bool force)
        {

            Dispose();
        }

        public bool IsEnabled
        {
            get { return FInEnabled.SliceCount > 0 && FInEnabled[0] == true; }
        }

        public bool IsVisible
        {
            get { return true; }
        }

        public void Present()
        {
            if (primary!=null) primary.Device.ImmediateContext.Flush();
            for (int i = 0; i < FInstances.SliceCount; i++)
            {
                var instance = FInstances[i];
                if (instance == null)
                    continue;

                instance.OnPresent();
                if (this.FInSyncLoop[i])
                {
                    instance.WaitSync();
                }
            }
        }

        public DX11RenderContext RenderContext
        {
            get
            {
                return this.primary;
            }
            set
            {
                this.primary = value;
            }
        }

        public IntPtr WindowHandle
        {
            get { return IntPtr.Zero; }
        }
    }
}
