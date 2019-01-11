using System;
using System.Collections.Generic;
using System.ComponentModel.Composition;
using System.Runtime.InteropServices;
using System.Linq;
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

namespace VVVV.Nodes.DX11PlayerNode
{
    enum Status
    {
        Init = 0,
        Ready,
        FirstFrame,
        Error = -1
    };
    [PluginInfo(Name     = "Player",
                Category = "DX11.Texture",
                Author   = "NSYNK GmbH")]
    public class DX11PlayerNode : 
        IPluginEvaluate, 
        IDX11ResourceHost, 
        IDisposable,
        IPartImportsSatisfiedNotification
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
                SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_APPLICATION_DIR |
                                         LOAD_LIBRARY_SEARCH_USER_DIRS |
                                         LOAD_LIBRARY_SEARCH_DEFAULT_DIRS |
                                         LOAD_LIBRARY_SEARCH_SYSTEM32);
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
        [Input("Format file", StringType = StringType.Filename)]
        public IDiffSpread<string> FFormatFile;

        [Input("Load files")]
        public IDiffSpread<ISpread<string>> FFileLoadIn;

        [Input("Frame render index")]
        public IDiffSpread<ISpread<int>> FNextFrameRenderIn;

        [Input("Always show last frame")]
        public IDiffSpread<bool> FAlwaysShowLastIn;

        [Output("Status")]
        public ISpread<string> FStatusOut;

        [Output("Texture")]
        public ISpread<ISpread<DX11Resource<DX11Texture2D>>> FTextureOut;

        [Output("Resolution")]
        public ISpread<Vector2D> FSizeOut;

        [Output("Texture format")]
        public ISpread<Format> FFormatOut;

        [Output("Upload buffer size")]
        public ISpread<int> FUploadSizeOut;

        [Output("Wait buffer size")]
        public ISpread<int> FWaitSizeOut;

        [Output("Render buffer size")]
        public ISpread<int> FRenderSizeOut;

        [Output("Present buffer size")]
        public ISpread<int> FPresentSizeOut;

        [Output("Number of dropped frames")]
        public ISpread<int> FDroppedFramesOut;

        [Output("Average load duration ms")]
        public ISpread<int> FAvgLoadDurationMsOut;

        [Output("Ready state")]
        public ISpread<bool> FGotFirstFrameOut;

        [Output("Load frame")]
        public ISpread<string> FLoadFrameOut;

        [Output("Render frame")]
        public ISpread<string> FRenderFrameOut;

        [Output("Got requested frame")]
        public ISpread<bool> FGotRequestedFrameOut;

        private Spread<IntPtr> FDX11NativePlayer = new Spread<IntPtr>();
        private Spread<string> FPrevFormatFile = new Spread<string>();
        private Spread<int> FPrevRenderFrameIdxCount = new Spread<int>();
        private Spread<ISpread<string>> FPrevFrameRendered = new Spread<ISpread<string>>();
        private Spread<bool> FIsReady = new Spread<bool>();
        private WorkerThread FWorkerThread = new WorkerThread();
        private Spread<ISpread<string>> FPrevFrames = new Spread<ISpread<string>>();
        private Spread<int> FPrevRenderFrames = new Spread<int>();
        private ISpread<Dictionary<IntPtr, DX11Resource<DX11Texture2D>>> FSharedTextureCache = new Spread<Dictionary<IntPtr, DX11Resource<DX11Texture2D>>>();
        private int FPrevNumSpreads = 0;
        private bool FRefreshPlayer = false;
        [Import]
        ILogger FLogger;

#pragma warning restore

        private bool FRefreshTextures = false;

        private void CreateTextureOut(int i)
        {
            FTextureOut[i] = new Spread<DX11Resource<DX11Texture2D>>();
            var NumSpreads = Math.Max(FNextFrameRenderIn[i].SliceCount, 1);
            FTextureOut[i].SliceCount = NumSpreads;
            FPrevFrameRendered[i].SliceCount = NumSpreads;
            FPrevRenderFrameIdxCount[i] = NumSpreads;
            for (int j = 0; j < NumSpreads; j++)
            {
                FTextureOut[i][j] = new DX11Resource<DX11Texture2D>();
            }
        }

        private void DestroyPlayer(int i)
        {
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
                    catch (Exception e)
                    {
                        FLogger.Log(LogType.Error, "Exception: ", e.Message);
                    }
                }
                FSharedTextureCache[i].Clear();
            }
            catch (Exception e)
            {
                FLogger.Log(LogType.Error, "Exception", e.Message);
            }

            if (FDX11NativePlayer[i] != IntPtr.Zero)
            {
                NativeInterface.DX11Player_Destroy(FDX11NativePlayer[i]);
                FDX11NativePlayer[i] = IntPtr.Zero;
            }

            FIsReady[i] = false;
            FGotFirstFrameOut[i] = false;
            CreateTextureOut(i);
            FPrevFrames[i].SliceCount = FFileLoadIn[i].SliceCount;
            for (int j = 0; j < FPrevFrames[i].Count(); j++ )
            {
                FPrevFrames[i][j] = "";
            }

            FRefreshTextures = true;
            FRefreshPlayer = false;
        }

        public void Evaluate(int spreadMax)
        {
            try
            {
                var NumSpreads = Math.Max(FFormatFile.SliceCount, 1);
                if (FPrevNumSpreads != NumSpreads)
                {
                    FLogger.Log(LogType.Message, "Evaluate: Setting spreads to " + NumSpreads);
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
                    FLoadFrameOut.SliceCount = NumSpreads;
                    FRenderFrameOut.SliceCount = NumSpreads;
                    FSizeOut.SliceCount = NumSpreads;
                    FFormatOut.SliceCount = NumSpreads;
                    FPrevFormatFile.SliceCount = NumSpreads;
                    FPrevFrameRendered.SliceCount = NumSpreads;
                    FPrevRenderFrameIdxCount.SliceCount = NumSpreads;
                    FGotFirstFrameOut.SliceCount = NumSpreads;
                    for (int i = 0; i < FDX11NativePlayer.SliceCount; i++)
                    {
                        FPrevFrames[i] = new Spread<string>();
                        FPrevFrameRendered[i] = new Spread<string>();
                        OutputDebugString("Evaluate: Destroying player - number of spreads changed");
                        DestroyPlayer(i);
                        FSharedTextureCache[i] = new Dictionary<IntPtr,DX11Resource<DX11Texture2D>>();
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
                    }
                    FStatusOut[i] = NativeInterface.DX11Player_GetStatusMessage(FDX11NativePlayer[i]);
                }
                else
                {
                    //FStatusOut[i] = "Not set";
                }
            }
            
            if(FPrevFormatFile.Count() == 0)
            {
                FPrevFormatFile.SliceCount = FFormatFile.Count();
            }
            
            for (int i=0; i<FFormatFile.Count(); i++)
            {
                if (FFormatFile[i] != FPrevFormatFile[i])
                {
                    if (FFormatFile[i] != null &&
                        ((FPrevFormatFile[i] == "" || FPrevFormatFile[i] == null) || !NativeInterface.DX11Player_IsSameFormat(FFormatFile[i], FPrevFormatFile[i])))
                    {
                        OutputDebugString("Destroying player format file changed");
                        OutputDebugString("Previous " + FPrevFormatFile[i]);
                        OutputDebugString("Current " + FFormatFile[i]);
                        FRefreshPlayer = true;
                    }
                    else
                    {
                        OutputDebugString("File format changed but format is the same, won't reallocate");
                        OutputDebugString("Previous " + FPrevFormatFile[i]);
                        OutputDebugString("Current " + FFormatFile[i]);
                    }
                    FPrevFormatFile[i] = FFormatFile[i];
                }
            }

            if (FFormatFile.Count() == 0)
            {
                for (int i = 0; i < FPrevFormatFile.Count(); i++)
                {
                    FPrevFormatFile[i] = null;
                }
                FPrevFormatFile.SliceCount = 0;
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

            for (int i = 0; i < FNextFrameRenderIn.SliceCount; i++)
            {
                if (FNextFrameRenderIn[i].Count() != FPrevRenderFrameIdxCount[i])
                {
                    CreateTextureOut(i);
                    FPrevRenderFrameIdxCount[i] = FNextFrameRenderIn[i].Count();
                }
            }

            if (FRefreshPlayer)
            {
                for (int i = 0; i < FDX11NativePlayer.SliceCount; i++)
                {
                    DestroyPlayer(i);
                }
            }

        }

        public void Update(DX11RenderContext context)
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

                    string nextFile = "";
                    if (FFileLoadIn[i].Count() > 0)
                    {
                        for(int j=0; j < FNextFrameRenderIn[i].Count(); j++)
                        {
                            var nextFrameRenderIdx = FNextFrameRenderIn[i][j];
                            var prevFrameRender = FPrevFrameRendered[i][j];
                            var renderFrame = (Math.Max(0, nextFrameRenderIdx) % FFileLoadIn[i].Count());
                            nextFile = FFileLoadIn[i][renderFrame];
                            if (prevFrameRender != nextFile)
                            {
                                var handle = NativeInterface.DX11Player_GetSharedHandle(FDX11NativePlayer[i], nextFile);
                                if (handle != IntPtr.Zero)
                                {
                                    if (FSharedTextureCache[i].ContainsKey(handle) &&
                                        FSharedTextureCache[i][handle].Contains(context))
                                    {
                                        FTextureOut[i][j] = FSharedTextureCache[i][handle];
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
                                        FTextureOut[i][j] = this.FSharedTextureCache[i][handle];
                                    }
                                    FSizeOut[i] = new Vector2D(FTextureOut[i][j][context].Width,
                                        FTextureOut[i][j][context].Height);
                                    FFormatOut[i] = FTextureOut[i][j][context].Format;
                                    FGotFirstFrameOut[i] = true;
                                    FPrevFrameRendered[i][j] = nextFile;
                                }
                            }
                        }
                    }

                    FRenderFrameOut[i] = NativeInterface.DX11Player_GetCurrentRenderFrame(FDX11NativePlayer[i]);
                    FGotRequestedFrameOut[i] = nextFile == FRenderFrameOut[i];
                    NativeInterface.DX11Player_Update(FDX11NativePlayer[i]);

                    if (this.FIsReady[i])
                    {
                        NativeInterface.DX11Player_SetSystemFrames(FDX11NativePlayer[i], FFileLoadIn[i].ToArray(), FFileLoadIn[i].Count());
                        var diff = FFileLoadIn[i].Except(FPrevFrames[i]);
                        foreach (var f in diff)
                        {
                            NativeInterface.DX11Player_SendNextFrameToLoad(FDX11NativePlayer[i], f);
                        }
                        if (FDX11NativePlayer[i] != IntPtr.Zero && FPrevFrames[i].Count() != FFileLoadIn[i].Count())
                        {
                            FRefreshPlayer = true;
                            OutputDebugString("Destroying player number of files to load has changed");
                        }
                        FPrevFrames[i] = FFileLoadIn[i].Clone();
                        FLoadFrameOut[i] = NativeInterface.DX11Player_GetCurrentLoadFrame(FDX11NativePlayer[i]);
                    }
                }
            }
        }

        public void Destroy(DX11RenderContext context, bool force)
        {
            for (int i = 0; i < FDX11NativePlayer.SliceCount; i++)
            {
                foreach (var handle in this.FSharedTextureCache[i].ToList())
                {
                    if (handle.Value.Contains(context))
                    {
                        handle.Value.Dispose();
                        FSharedTextureCache[i].Remove(handle.Key);
                    }
                }
                for (int j = 0; j < FTextureOut[i].Count(); j++)
                {
                    if (this.FTextureOut[i][j].Contains(context))
                    {
                        this.FTextureOut[i][j] = null;
                    }
                }
            }
        }

        public void Dispose()
        {
            OutputDebugString("Dispose: Disposing DX11Player plugin");
            for (int i = 0; i < FDX11NativePlayer.SliceCount; i++)
            {
                if (FDX11NativePlayer[i] != IntPtr.Zero)
                {
                    NativeInterface.DX11Player_Destroy(FDX11NativePlayer[i]);
                    FDX11NativePlayer[i] = IntPtr.Zero;
                    foreach(var texture in FSharedTextureCache[i].ToList())
                    {
                        texture.Value.Dispose();
                        FSharedTextureCache[i].Remove(texture.Key);
                    }
                    this.FIsReady[i] = false;
                    this.FGotFirstFrameOut[i] = false;
                }
            }
        }

        public void OnImportsSatisfied()
        {
            // Check existance of DX11NativePlayer.dll
            try
            {
                NativeInterface.DX11Player_Available();
            }
            catch (System.DllNotFoundException e) {
                FLogger.Log(LogType.Error, e.Message);
                FStatusOut[0] = e.Message;
            }
        }
    }


    internal class NativeInterface
    {
        [DllImport("DX11PlayerNative.dll", SetLastError = false, CharSet = CharSet.Ansi)]
        internal static extern bool DX11Player_Available();

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
