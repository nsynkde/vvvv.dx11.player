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
using SlimDX.DXGI;
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
    public class DX11PlayerNode : IPluginEvaluate, IDX11ResourceProvider, IDisposable
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
        [Input("format file", StringType = StringType.Filename)]
        public IDiffSpread<string> FFormatFile;

        [Input("load files")]
        public IDiffSpread<ISpread<string>> FFileLoadIn;

        [Input("frame render idx")]
        public IDiffSpread<int> FNextFrameRenderIn;

        [Input("always show last frame")]
        public IDiffSpread<bool> FAlwaysShowLastIn;

        [Output("status")]
        public ISpread<string> FStatusOut;

        [Output("texture")]
        public ISpread<DX11Resource<DX11Texture2D>> FTextureOut;

        [Output("size")]
        public ISpread<Vector2D> FSizeOut;

        [Output("tex format")]
        public ISpread<Format> FFormatOut;

        [Output("upload buffer size")]
        public ISpread<int> FUploadSizeOut;

        [Output("wait buffer size")]
        public ISpread<int> FWaitSizeOut;

        [Output("render buffer size")]
        public ISpread<int> FRenderSizeOut;

        [Output("present buffer size")]
        public ISpread<int> FPresentSizeOut;

        [Output("dropped frames")]
        public ISpread<int> FDroppedFramesOut;

        [Output("avg load duration ms")]
        public ISpread<int> FAvgLoadDurationMsOut;

        [Output("is ready")]
        public ISpread<bool> FGotFirstFrameOut;

        [Output("load frame")]
        public ISpread<string> FLoadFrameOut;

        [Output("render frame")]
        public ISpread<string> FRenderFrameOut;

        private Spread<IntPtr> FDX11NativePlayer = new Spread<IntPtr>();
        private Spread<string> FPrevFormatFile = new Spread<string>();
        private Spread<bool> FIsReady = new Spread<bool>();
        private WorkerThread FWorkerThread = new WorkerThread();
        private Spread<ISpread<string>> FPrevFrames = new Spread<ISpread<string>>();
        private Spread<int> FPrevRenderFrames = new Spread<int>();
        private Spread<bool> FGetNextFrame = new Spread<bool>();
        private ISpread<Dictionary<IntPtr, DX11Resource<DX11Texture2D>>> FSharedTextureCache = new Spread<Dictionary<IntPtr, DX11Resource<DX11Texture2D>>>();
        private int FPrevNumSpreads = 0;
        private bool FRefreshPlayer = false;
        [Import]
        ILogger FLogger;

#pragma warning restore

        private bool FRefreshTextures = false;

        private void DestroyPlayer(int i, bool cleanCache)
        {
            /*var nativePlayer = FDX11NativePlayer[i];
            if (nativePlayer != IntPtr.Zero)
            {
                FDX11NativePlayer[i] = IntPtr.Zero;
                FWorkerThread.Perform(() =>
                {
                    NativeInterface.DX11Player_Destroy(nativePlayer);
                });
            }*/
            if (FDX11NativePlayer[i] != IntPtr.Zero)
            {
                NativeInterface.DX11Player_Destroy(FDX11NativePlayer[i]);
                FDX11NativePlayer[i] = IntPtr.Zero;
            }

            try
            {
                foreach (var tex in FSharedTextureCache[i])
                {
                    try
                    {
                        if (tex.Value != null)
                        {
                            tex.Value.Dispose();
                        }
                    }
                    catch (Exception)
                    {
                    }
                }
                FSharedTextureCache[i].Clear();
            }
            catch (Exception)
            {
            }
            FIsReady[i] = false;
            FGotFirstFrameOut[i] = false;
            FTextureOut[i] = new DX11Resource<DX11Texture2D>();
            FPrevFrames[i].SliceCount = FFileLoadIn[i].SliceCount;
            for (int j = 0; j < FPrevFrames[i].Count(); j++ )
            {
                FPrevFrames[i][j] = "";
            }

        }

        public void Evaluate(int spreadMax)
        {
            try
            {
                var NumSpreads = Math.Max(FFormatFile.SliceCount, 1);
                if (FPrevNumSpreads != NumSpreads)
                {
                    FLogger.Log(LogType.Message, "setting spreads to " + NumSpreads);
                    FRefreshTextures = true;
                    FDX11NativePlayer.SliceCount = NumSpreads;
                    FStatusOut.SliceCount = NumSpreads;
                    FTextureOut.SliceCount = NumSpreads; ;
                    FUploadSizeOut.SliceCount = NumSpreads;
                    FWaitSizeOut.SliceCount = NumSpreads;
                    FRenderSizeOut.SliceCount = NumSpreads;
                    FPresentSizeOut.SliceCount = NumSpreads;
                    FDroppedFramesOut.SliceCount = NumSpreads;
                    FAvgLoadDurationMsOut.SliceCount = NumSpreads;
                    FIsReady.SliceCount = NumSpreads;
                    FGotFirstFrameOut.SliceCount = NumSpreads;
                    FPrevFrames.SliceCount = NumSpreads;
                    FSharedTextureCache.SliceCount = NumSpreads;
                    FPrevRenderFrames.SliceCount = NumSpreads;
                    FGetNextFrame.SliceCount = NumSpreads;
                    FLoadFrameOut.SliceCount = NumSpreads;
                    FRenderFrameOut.SliceCount = NumSpreads;
                    FSizeOut.SliceCount = NumSpreads;
                    FFormatOut.SliceCount = NumSpreads;
                    FPrevFormatFile.SliceCount = NumSpreads;
                    for (int i = 0; i < FDX11NativePlayer.SliceCount; i++)
                    {
                        FPrevFrames[i] = new Spread<string>();
                        OutputDebugString("Destroying player numspreads changed");
                        DestroyPlayer(i,false);
                        FSharedTextureCache[i] = new Dictionary<IntPtr,DX11Resource<DX11Texture2D>>();
                        FGetNextFrame[i] = false;
                        FIsReady[i] = false;
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
                        FPresentSizeOut[i] = NativeInterface.DX11Player_GetPresentBufferSize(FDX11NativePlayer[i]);
                        FDroppedFramesOut[i] = NativeInterface.DX11Player_GetDroppedFrames(FDX11NativePlayer[i]);
                        FAvgLoadDurationMsOut[i] = (int)(((double)FAvgLoadDurationMsOut[i]) * 0.9 + ((double)NativeInterface.DX11Player_GetAvgLoadDurationMs(FDX11NativePlayer[i])) * 0.1);
                        FLoadFrameOut[i] = NativeInterface.DX11Player_GetCurrentLoadFrame(FDX11NativePlayer[i]);
                        FRenderFrameOut[i] = NativeInterface.DX11Player_GetCurrentRenderFrame(FDX11NativePlayer[i]);
                    }

                    FStatusOut[i] = NativeInterface.DX11Player_GetStatusMessage(FDX11NativePlayer[i]);
                }
                else
                {
                    FStatusOut[i] = "Not set";
                }
            }

            if (FFormatFile.IsChanged)
            {
                for(int i=0;i< FPrevFormatFile.Count(); i++)
                {
                    if(FPrevFormatFile[i]!="" && FPrevFormatFile[i] != null && !NativeInterface.DX11Player_IsSameFormat(FFormatFile[i], FPrevFormatFile[i]))
                    {
                        FRefreshPlayer = true;
                    }
                    FPrevFormatFile[i] = FFormatFile[i];
                }
                if (FRefreshPlayer)
                {
                    OutputDebugString("Destroying player format file changed");
                }
                else
                {
                    OutputDebugString("File format changed but format is the same, won't reallocate");
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
                    }
                }

            }

            if (FAlwaysShowLastIn.IsChanged)
            {
                for (int i = 0; i < FDX11NativePlayer.SliceCount; i++)
                {
                    if (FDX11NativePlayer[i] != IntPtr.Zero)
                    {
                        NativeInterface.DX11Player_SetAlwaysShowLastFrame(FDX11NativePlayer[i], FAlwaysShowLastIn[i]);
                    }
                }

            }

            if (FRefreshPlayer)
            {
                for (int i = 0; i < FDX11NativePlayer.SliceCount; i++)
                {
                    if (FDX11NativePlayer[i] != IntPtr.Zero)
                    {
                        DestroyPlayer(i,true);
                    }
                }
                FRefreshTextures = true;
                FRefreshPlayer = false;
            }

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
                            var nativePlayer = NativeInterface.DX11Player_Create(FFormatFile[i], FFileLoadIn[i].Count());
                            if (nativePlayer != IntPtr.Zero)
                            {
                                NativeInterface.DX11Player_SetAlwaysShowLastFrame(nativePlayer, FAlwaysShowLastIn[i]);
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
                    if (!FIsReady[i] && NativeInterface.DX11Player_IsReady(FDX11NativePlayer[i]))
                    {
                        FIsReady[i] = true;
                    }

                    NativeInterface.DX11Player_Update(FDX11NativePlayer[i]);

                    if (FGetNextFrame[i] && FFileLoadIn[i].Count()>0)
                    {
                        var renderFrame = (Math.Max(0,FNextFrameRenderIn[i]) % (FFileLoadIn[i].Count()-2));
                        var handle = NativeInterface.DX11Player_GetSharedHandle(FDX11NativePlayer[i], FFileLoadIn[i][renderFrame]);

                        if (handle != IntPtr.Zero)
                        {
                            if (FSharedTextureCache[i].ContainsKey(handle) && FSharedTextureCache[i][handle].Contains(context))
                            {
                                FTextureOut[i] = FSharedTextureCache[i][handle];
                            }
                            else
                            {
                                Texture2D tex = context.Device.OpenSharedResource<Texture2D>(handle);
                                ShaderResourceView srv = new ShaderResourceView(context.Device, tex);
                                DX11Texture2D resource = DX11Texture2D.FromTextureAndSRV(context, tex, srv);
                                if (!FSharedTextureCache[i].ContainsKey(handle))
                                {
                                    FSharedTextureCache[i][handle] = new DX11Resource<DX11Texture2D>();
                                }
                                else if (FSharedTextureCache[i][handle].Contains(context))
                                {
                                    FSharedTextureCache[i][handle][context].Dispose();
                                }
                                FSharedTextureCache[i][handle][context] = resource;
                                FTextureOut[i] = this.FSharedTextureCache[i][handle];
                            }
                            FSizeOut[i] = new Vector2D(this.FTextureOut[i][context].Width,FTextureOut[i][context].Height);
                            FFormatOut[i] = FTextureOut[i][context].Format;
                            FGotFirstFrameOut[i] = true;
                            if (FFileLoadIn[i][renderFrame] == NativeInterface.DX11Player_GetCurrentRenderFrame(FDX11NativePlayer[i]))
                            {
                                FGetNextFrame[i] = false;
                            }
                        }
                    }

                    if (this.FIsReady[i])
                    {
                        NativeInterface.DX11Player_SetSystemFrames(FDX11NativePlayer[i], FFileLoadIn[i].ToArray(), FFileLoadIn[i].Count());
                        var diff = FFileLoadIn[i].Except(FPrevFrames[i]);
                        foreach (var f in diff)
                        {
                            NativeInterface.DX11Player_SendNextFrameToLoad(FDX11NativePlayer[i], f);
                            FGetNextFrame[i] = true;
                        }
                        if (FDX11NativePlayer[i] != IntPtr.Zero && FPrevFrames[i].Count() != FFileLoadIn[i].Count())
                        {
                            FRefreshPlayer = true;
                            OutputDebugString("Destroying player number of files to load has changed");
                        }
                        FPrevFrames[i] = FFileLoadIn[i].Clone();
                    }
                }
            }
        }

        public void Destroy(IPluginIO pin, DX11RenderContext context, bool force)
        {
            for (int i = 0; i < FDX11NativePlayer.SliceCount; i++)
            {
                foreach (var handle in this.FSharedTextureCache[i])
                {
                    if (handle.Value.Contains(context))
                    {
                        handle.Value.Dispose();
                        FSharedTextureCache[i].Remove(handle.Key);
                    }
                }
                if (this.FTextureOut[i].Contains(context))
                {
                    this.FTextureOut[i] = null;
                }
            }
        }

        public void Dispose()
        {
            OutputDebugString("Disposing DX11Player plugin");
            for (int i = 0; i < FDX11NativePlayer.SliceCount; i++)
            {
                if (FDX11NativePlayer[i] != IntPtr.Zero)
                {
                    NativeInterface.DX11Player_Destroy(FDX11NativePlayer[i]);
                    FDX11NativePlayer[i] = IntPtr.Zero;
                    foreach(var tex in FSharedTextureCache[i]){
                        tex.Value.Dispose();
                        FSharedTextureCache[i].Remove(tex.Key);
                    }
                    //this.FTextureOut[i].Dispose();
                    this.FIsReady[i] = false;
                    this.FGotFirstFrameOut[i] = false;
                }
            }
        }
    }


    internal class NativeInterface
    {
        [DllImport("DX11PlayerNative.dll", SetLastError = false, CharSet = CharSet.Ansi)]
        internal static extern IntPtr DX11Player_Create(string formatFile, int ringBufferSize);

        [DllImport("DX11PlayerNative.dll", SetLastError = false)]
        internal static extern void DX11Player_Destroy(IntPtr player);

        [DllImport("DX11PlayerNative.dll", SetLastError = false)]
        internal static extern void DX11Player_Update(IntPtr player);

        [DllImport("DX11PlayerNative.dll", SetLastError = false)]
        internal static extern IntPtr DX11Player_GetSharedHandle(IntPtr player, string nextFrame);

        [DllImport("DX11PlayerNative.dll", SetLastError = false)]
        internal static extern int DX11Player_GetUploadBufferSize(IntPtr player);

        [DllImport("DX11PlayerNative.dll", SetLastError = false)]
        internal static extern int DX11Player_GetWaitBufferSize(IntPtr player);

        [DllImport("DX11PlayerNative.dll", SetLastError = false)]
        internal static extern int DX11Player_GetRenderBufferSize(IntPtr player);

        [DllImport("DX11PlayerNative.dll", SetLastError = false)]
        internal static extern int DX11Player_GetPresentBufferSize(IntPtr player);

        [DllImport("DX11PlayerNative.dll", SetLastError = false)]
        internal static extern int DX11Player_GetDroppedFrames(IntPtr player);

        [DllImport("DX11PlayerNative.dll", SetLastError = false)]
        internal static extern int DX11Player_GetAvgLoadDurationMs(IntPtr player);

        [DllImport("DX11PlayerNative.dll", SetLastError = false)]
        internal static extern void DX11Player_SendNextFrameToLoad(IntPtr player, string nextFrame);

        [DllImport("DX11PlayerNative.dll", SetLastError = false)]
        [return: MarshalAs(UnmanagedType.I1)]
        internal static extern bool DX11Player_IsReady(IntPtr player);

        [DllImport("DX11PlayerNative.dll", SetLastError = false)]
        [return: MarshalAs(UnmanagedType.I1)]
        internal static extern bool DX11Player_GotFirstFrame(IntPtr player);

        [DllImport("DX11PlayerNative.dll", SetLastError = false)]
        internal static extern int DX11Player_GetStatus(IntPtr player);

        [DllImport("DX11PlayerNative.dll", SetLastError = false, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.StdCall)]
        [return: MarshalAs(UnmanagedType.LPStr)]
        internal static extern string DX11Player_GetStatusMessage(IntPtr player);

        [DllImport("DX11PlayerNative.dll", SetLastError = false, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.StdCall)]
        [return: MarshalAs(UnmanagedType.LPStr)]
        internal static extern string DX11Player_GetCurrentLoadFrame(IntPtr player);

        [DllImport("DX11PlayerNative.dll", SetLastError = false, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.StdCall)]
        [return: MarshalAs(UnmanagedType.LPStr)]
        internal static extern string DX11Player_GetCurrentRenderFrame(IntPtr player);

        [DllImport("DX11PlayerNative.dll", CallingConvention = CallingConvention.StdCall)]
        internal static extern void DX11Player_SetSystemFrames(IntPtr player, String[] frames, int size);

        [DllImport("DX11PlayerNative.dll")]
        internal static extern void DX11Player_SetAlwaysShowLastFrame(IntPtr player, bool always);


        [DllImport("DX11PlayerNative.dll", SetLastError = false, CharSet = CharSet.Ansi)]
        internal static extern bool DX11Player_IsSameFormat(string formatFile1, string formatFile2);
    }

}
