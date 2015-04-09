using System;
using System.Collections.Generic;
using System.ComponentModel.Composition;
using System.Runtime.InteropServices;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using VVVV.Core.Logging;
using VVVV.Utils.VMath;
using System.Reflection;
using System.IO;
using FeralTic.DX11;
using FeralTic.DX11.Resources;

using VVVV.PluginInterfaces.V2;
using VVVV.PluginInterfaces.V1;
using SlimDX.Direct3D11;
using VVVV.DX11;
using VVVV.Nodes;

namespace VVVV.Nodes.DX11PlayerNode
{
    enum Status
    {
        Init = 0,
        Ready,
        FirstFrame,
        Error = -1
    };
    [PluginInfo(Name = "Player", Category = "DX11.Texture")]
    public class DX11PlayerNode : IPluginEvaluate, IDX11ResourceProvider
    {
        

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

        static DX11PlayerNode()
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
                OutputDebugString("Adding " + pluginfolder + " to dll search path");
                AddDllDirectory(pluginfolder);
            }
            catch (Exception e)
            {
                OutputDebugString("Error adding dll search path" + e.StackTrace);
            }

        }

#pragma warning disable 0649
        [Input("directory", StringType = StringType.Directory)]
        public IDiffSpread<string> FDirectoryIn;

        [Input("filemask", DefaultString = "*")]
        public IDiffSpread<string> FFileMaskIn;

        [Input("fps", DefaultValue = 60)]
        public IDiffSpread<int> FFPSIn;

        [Input("internal rate enabled", DefaultValue = 1)]
        public IDiffSpread<bool> FInternalRate;

        [Input("load frame")]
        public IDiffSpread<ISpread<int>> FFrameLoadIn;

        [Input("next frame render")]
        public IDiffSpread<int> FNextFrameRenderIn;

        [Output("status")]
        public ISpread<string> FStatusOut;

        [Output("texture")]
        public ISpread<DX11Resource<DX11Texture2D>> FTextureOut;

        [Output("upload buffer size")]
        public ISpread<int> FUploadSizeOut;

        [Output("wait buffer size")]
        public ISpread<int> FWaitSizeOut;

        [Output("render buffer size")]
        public ISpread<int> FRenderSizeOut;

        [Output("dropped frames")]
        public ISpread<int> FDroppedFramesOut;

        [Output("load frame")]
        public ISpread<int> FLoadFrameOut;

        [Output("render frame")]
        public ISpread<int> FRenderFrameOut;

        [Output("avg load duration ms")]
        public ISpread<int> FAvgLoadDurationMsOut;

        [Output("is ready")]
        public ISpread<bool> FGotFirstFrameOut;

        [Output("waiting next frame")]
        public ISpread<bool> FGetNextFrame;

        [Output("waiting frame idx")]
        public ISpread<int> FWaitingFrameIdx;

        [Output("context pool size")]
        public ISpread<int> FContextPoolSize;

        private Spread<IntPtr> FDX11NativePlayer = new Spread<IntPtr>();
        private Spread<bool> FIsReady = new Spread<bool>();
        private WorkerThread FWorkerThread = new WorkerThread();
        private Spread<ISpread<int>> FPrevFrames = new Spread<ISpread<int>>();
        private Spread<int> FPrevRenderFrames = new Spread<int>();
        private ISpread<Dictionary<IntPtr, DX11Resource<DX11Texture2D>>> FSharedTextureCache = new Spread<Dictionary<IntPtr, DX11Resource<DX11Texture2D>>>();
        private int FPrevNumSpreads = 0;
        [Import]
        ILogger FLogger;

#pragma warning restore

        private bool FRefreshTextures = false;

        private void DestroyPlayer(int i)
        {
            var nativePlayer = FDX11NativePlayer[i];
            if (nativePlayer != IntPtr.Zero)
            {
                FDX11NativePlayer[i] = IntPtr.Zero;
                FWorkerThread.Perform(() =>
                {
                    NativeInterface.DX11Player_Destroy(nativePlayer);
                });
            }
            this.FIsReady[i] = false;
            this.FGotFirstFrameOut[i] = false;
            this.FTextureOut[i] = new DX11Resource<DX11Texture2D>();
            this.FPrevFrames[i].SliceCount = this.FFrameLoadIn[i].SliceCount;
            for (int j = 0; j < this.FPrevFrames[i].Count(); j++ )
            {
                this.FPrevFrames[i][j] = -1;
            }

        }

        public void Evaluate(int spreadMax)
        {
            try
            {
                var NumSpreads = Math.Max(FDirectoryIn.SliceCount, 1);
                if (FPrevNumSpreads != NumSpreads)
                {
                    FRefreshTextures = true;
                    FDX11NativePlayer.SliceCount = NumSpreads;
                    FStatusOut.SliceCount = NumSpreads;
                    FTextureOut.SliceCount = NumSpreads; ;
                    FUploadSizeOut.SliceCount = NumSpreads;
                    FWaitSizeOut.SliceCount = NumSpreads;
                    FRenderSizeOut.SliceCount = NumSpreads;
                    FDroppedFramesOut.SliceCount = NumSpreads;
                    FLoadFrameOut.SliceCount = NumSpreads;
                    FRenderFrameOut.SliceCount = NumSpreads;
                    FAvgLoadDurationMsOut.SliceCount = NumSpreads;
                    FIsReady.SliceCount = NumSpreads;
                    FGotFirstFrameOut.SliceCount = NumSpreads;
                    FPrevFrames.SliceCount = NumSpreads;
                    FSharedTextureCache.SliceCount = NumSpreads;
                    FGetNextFrame.SliceCount = NumSpreads;
                    FPrevRenderFrames.SliceCount = NumSpreads;
                    FContextPoolSize.SliceCount = 1;
                    FWaitingFrameIdx.SliceCount = NumSpreads;
                    for (int i = 0; i < FDX11NativePlayer.SliceCount; i++)
                    {
                        this.FPrevFrames[i] = new Spread<int>();
                        DestroyPlayer(i);
                        FSharedTextureCache[i] = new Dictionary<IntPtr,DX11Resource<DX11Texture2D>>();
                        FGetNextFrame[i] = false;
                    }
                    FPrevNumSpreads = NumSpreads;
                }
            }
            catch (Exception e)
            {
                FLogger.Log(LogType.Error, e.StackTrace);
                throw e;
            }
            
            for (int i = 0; i < FDX11NativePlayer.SliceCount; i++)
            {
                if (FDX11NativePlayer[i] != IntPtr.Zero)
                {
                    if (this.FIsReady[i])
                    {
                        FUploadSizeOut[i] = NativeInterface.DX11Player_GetUploadBufferSize(FDX11NativePlayer[i]);
                        FWaitSizeOut[i] = NativeInterface.DX11Player_GetWaitBufferSize(FDX11NativePlayer[i]);
                        FRenderSizeOut[i] = NativeInterface.DX11Player_GetRenderBufferSize(FDX11NativePlayer[i]);
                        FDroppedFramesOut[i] = NativeInterface.DX11Player_GetDroppedFrames(FDX11NativePlayer[i]);
                        FLoadFrameOut[i] = NativeInterface.DX11Player_GetCurrentLoadFrame(FDX11NativePlayer[i]);
                        FRenderFrameOut[i] = NativeInterface.DX11Player_GetCurrentRenderFrame(FDX11NativePlayer[i]);
                        FAvgLoadDurationMsOut[i] = (int)(((double)FAvgLoadDurationMsOut[i]) * 0.9 + ((double)NativeInterface.DX11Player_GetAvgLoadDurationMs(FDX11NativePlayer[i])) * 0.1);
                        if (FFPSIn.IsChanged)
                        {
                            NativeInterface.DX11Player_SetFPS(FDX11NativePlayer[i], FFPSIn[i]);
                        }
                    }

                    FStatusOut[i] = NativeInterface.DX11Player_GetStatusMessage(FDX11NativePlayer[i]);
                }
                else
                {
                    FStatusOut[i] = "Not set";
                }
            }

            if (FDirectoryIn.IsChanged)
            {
                for (int i = 0; i < FDX11NativePlayer.SliceCount; i++)
                {
                    if (FDX11NativePlayer[i] != IntPtr.Zero)
                    {
                        if (NativeInterface.DX11Player_DirectoryHasChanged(FDX11NativePlayer[i], FDirectoryIn[i]) == 1)
                        {
                            DestroyPlayer(i);
                        }
                    }
                    this.FRefreshTextures = true;
                }
            }

            if (FFileMaskIn.IsChanged)
            {
                for (int i = 0; i < FDX11NativePlayer.SliceCount; i++)
                {
                    if (FDX11NativePlayer[i] != IntPtr.Zero)
                    {
                        DestroyPlayer(i);
                    }
                    this.FRefreshTextures = true;
                }
            }

            for (int i = 0; i < FDX11NativePlayer.SliceCount; i++)
            {
                if (this.FIsReady[i])
                {
                    var diff = FFrameLoadIn[i].Except(FPrevFrames[i]);
                    foreach (var f in diff)
                    {
                        NativeInterface.DX11Player_SendNextFrameToLoad(FDX11NativePlayer[i], f);
                        FGetNextFrame[i] = true;
                    }
                    if (FDX11NativePlayer[i] != IntPtr.Zero && FPrevFrames[i].Count() != FFrameLoadIn[i].Count())
                    {
                        DestroyPlayer(i);
                        this.FRefreshTextures = true;
                    }
                    FPrevFrames[i] = FFrameLoadIn[i].Clone();
                    FWaitingFrameIdx[i] = FFrameLoadIn[i][FNextFrameRenderIn[i]];
                }
            }

            if (FNextFrameRenderIn.IsChanged)
            {
                for (int i = 0; i < FPrevRenderFrames.SliceCount; i++)
                {
                    if (FPrevRenderFrames[i] != FNextFrameRenderIn[i])
                    {
                        FGetNextFrame[i] = true;
                        FPrevRenderFrames[i] = FNextFrameRenderIn[i];
                        FWaitingFrameIdx[i] = FFrameLoadIn[i][FNextFrameRenderIn[i]];
                    }
                }

            }

            if (FInternalRate.IsChanged)
            {
                for (int i = 0; i < FDX11NativePlayer.SliceCount; i++)
                {
                    if (this.FIsReady[i])
                    {
                        NativeInterface.DX11Player_SetInternalRate(FDX11NativePlayer[i], FInternalRate[i] ? 1 : 0);
                    }
                }
            }

            FContextPoolSize[0] = NativeInterface.DX11Player_GetContextPoolSize();

        }

        public void Update(IPluginIO pin, DX11RenderContext context)
        {
            if (FRefreshTextures)
            {
                FRefreshTextures = false;
                try
                {
                    for (int i = 0; i < FDX11NativePlayer.SliceCount; i++)
                    {
                        if (FDX11NativePlayer[i] == IntPtr.Zero)
                        {
                            var nativePlayer = NativeInterface.DX11Player_Create(FDirectoryIn[i], FFileMaskIn[i], FFrameLoadIn[i].Count());
                            if (nativePlayer != IntPtr.Zero)
                            {
                                NativeInterface.DX11Player_SetFPS(nativePlayer, FFPSIn[i]);
                                NativeInterface.DX11Player_SetInternalRate(nativePlayer, FInternalRate[i] ? 1 : 0);
                                FDX11NativePlayer[i] = nativePlayer;
                            }
                            else
                            {
                                throw new Exception("Creating native player returned nullptr");
                            }
                        }
                    }
                }
                catch (Exception ex)
                {
                    FLogger.Log(ex);
                }
            }

            for (int i = 0; i < FDX11NativePlayer.SliceCount; i++)
            {
                if (FDX11NativePlayer[i] != IntPtr.Zero)
                {
                    if (NativeInterface.DX11Player_IsReady(FDX11NativePlayer[i]) && !FIsReady[i])
                    {
                        FIsReady[i] = true;
                    }
                    NativeInterface.DX11Player_Update(FDX11NativePlayer[i]);
                    if (FGetNextFrame[i])
                    {
                        var renderFrame = (Math.Max(0,FNextFrameRenderIn[i]) % (FFrameLoadIn[i].Count()-2));
                        var handle = NativeInterface.DX11Player_GetSharedHandle(FDX11NativePlayer[i], FFrameLoadIn[i][renderFrame]);

                        if (handle != IntPtr.Zero)
                        {
                            if (FSharedTextureCache[i].ContainsKey(handle))
                            {
                                this.FTextureOut[i] = FSharedTextureCache[i][handle];
                            }
                            else
                            {
                                Texture2D tex = context.Device.OpenSharedResource<Texture2D>(handle);
                                ShaderResourceView srv = new ShaderResourceView(context.Device, tex);
                                DX11Texture2D resource = DX11Texture2D.FromTextureAndSRV(context, tex, srv);
                                this.FSharedTextureCache[i][handle] = new DX11Resource<DX11Texture2D>();
                                this.FSharedTextureCache[i][handle][context] = resource;
                                this.FTextureOut[i] = this.FSharedTextureCache[i][handle];
                            }
                            this.FGotFirstFrameOut[i] = true;
                            FGetNextFrame[i] = false;
                        }
                    }
                }
            }
        }

        public void Destroy(IPluginIO pin, DX11RenderContext context, bool force)
        {
            for (int i = 0; i < FDX11NativePlayer.SliceCount; i++)
            {
                if (FDX11NativePlayer[i] != IntPtr.Zero)
                {
                    NativeInterface.DX11Player_Destroy(FDX11NativePlayer[i]);
                    FDX11NativePlayer[i] = IntPtr.Zero;
                    this.FTextureOut[i] = new DX11Resource<DX11Texture2D>();
                    this.FIsReady[i] = false;
                    this.FGotFirstFrameOut[i] = false;
                }
            }
        }
    }


    internal class NativeInterface
    {
        [DllImport("DX11PlayerNative.dll", SetLastError = false, CharSet = CharSet.Ansi)]
        internal static extern IntPtr DX11Player_Create(string directory, string wildcard, int ringBufferSize);
        [DllImport("DX11PlayerNative.dll", SetLastError = false)]
        internal static extern void DX11Player_Destroy(IntPtr player);
        [DllImport("DX11PlayerNative.dll", SetLastError = false)]
        internal static extern void DX11Player_Update(IntPtr player);
        [DllImport("DX11PlayerNative.dll", SetLastError = false)]
        internal static extern IntPtr DX11Player_GetSharedHandle(IntPtr player, int nextFrame);
        [DllImport("DX11PlayerNative.dll", SetLastError = false, CharSet = CharSet.Ansi)]
        internal static extern string DX11Player_GetDirectory(IntPtr player);
        [DllImport("DX11PlayerNative.dll", SetLastError = false, CharSet = CharSet.Ansi)]
        internal static extern int DX11Player_DirectoryHasChanged(IntPtr player, string dir);
        [DllImport("DX11PlayerNative.dll", SetLastError = false)]
        internal static extern int DX11Player_GetUploadBufferSize(IntPtr player);
        [DllImport("DX11PlayerNative.dll", SetLastError = false)]
        internal static extern int DX11Player_GetWaitBufferSize(IntPtr player);
        [DllImport("DX11PlayerNative.dll", SetLastError = false)]
        internal static extern int DX11Player_GetRenderBufferSize(IntPtr player);
        [DllImport("DX11PlayerNative.dll", SetLastError = false)]
        internal static extern int DX11Player_GetDroppedFrames(IntPtr player);
        [DllImport("DX11PlayerNative.dll", SetLastError = false)]
        internal static extern int DX11Player_GetCurrentLoadFrame(IntPtr player);
        [DllImport("DX11PlayerNative.dll", SetLastError = false)]
        internal static extern int DX11Player_GetCurrentRenderFrame(IntPtr player);
        [DllImport("DX11PlayerNative.dll", SetLastError = false)]
        internal static extern int DX11Player_GetAvgLoadDurationMs(IntPtr player);
        [DllImport("DX11PlayerNative.dll", SetLastError = false)]
        internal static extern void DX11Player_SetFPS(IntPtr player,int fps);
        [DllImport("DX11PlayerNative.dll", SetLastError = false)]
        internal static extern void DX11Player_SendNextFrameToLoad(IntPtr player, int nextFrame);
        [DllImport("DX11PlayerNative.dll", SetLastError = false)]
        internal static extern void DX11Player_SetInternalRate(IntPtr player, int enabled);
        [DllImport("DX11PlayerNative.dll", SetLastError = false)]
        internal static extern bool DX11Player_IsReady(IntPtr player);
        [DllImport("DX11PlayerNative.dll", SetLastError = false)]
        internal static extern bool DX11Player_GotFirstFrame(IntPtr player);
        [DllImport("DX11PlayerNative.dll", SetLastError = false)]
        internal static extern int DX11Player_GetContextPoolSize();
        [DllImport("DX11PlayerNative.dll", SetLastError = false)]
        internal static extern int DX11Player_GetStatus(IntPtr player);
        [DllImport("DX11PlayerNative.dll", SetLastError = false, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.StdCall)]
        [return: MarshalAs(UnmanagedType.LPStr)]
        internal static extern string DX11Player_GetStatusMessage(IntPtr player);
    }

}
