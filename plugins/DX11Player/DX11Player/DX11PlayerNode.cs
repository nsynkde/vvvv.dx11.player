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
    [PluginInfo(Name = "Player", Category = "DX11.Texture", Author = "NSYNK GmbH")]

    public class DX11PlayerNode :
        IPluginEvaluate,
        IDX11ResourceHost,
        IDisposable,
        IPartImportsSatisfiedNotification
    {
        #region System library imports
        [DllImport("kernel32.dll", CharSet = CharSet.Auto)]
        private static extern void OutputDebugString(string message);

        [DllImport("kernel32.dll", CharSet = CharSet.Auto)]
        private static extern IntPtr AddDllDirectory(string newdirectory);

        [DllImport("kernel32.dll")]
        private static extern bool SetDefaultDllDirectories(UInt32 DirectoryFlags);
        #endregion

        #region Imports
        [Import] ILogger FLogger;
        #endregion

        #region Input pins

        [Input("Format file", StringType = StringType.Filename)]
        public IDiffSpread<string> FInFormatFile;

        [Input("Load files")]
        public IDiffSpread<ISpread<string>> FInTextureFiles;

        [Input("Frame render index")]
        public IDiffSpread<ISpread<int>> FNextFrameRenderIn;

        [Input("Always show last frame")]
        public IDiffSpread<bool> FAlwaysShowLastIn;
        #endregion

        #region Output pins
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

        [Output("Version", Visibility = PinVisibility.Hidden, IsSingle = true)]
        public ISpread<string> FVersion;
        #endregion

        #region class variables and properties
        const UInt32 LOAD_LIBRARY_SEARCH_APPLICATION_DIR = 0x00000200;
        const UInt32 LOAD_LIBRARY_SEARCH_USER_DIRS = 0x00000400;
        const UInt32 LOAD_LIBRARY_SEARCH_DEFAULT_DIRS = 0x00001000;
        const UInt32 LOAD_LIBRARY_SEARCH_SYSTEM32 = 0x00000800;

        private Spread<IntPtr> FDX11NativePlayer = new Spread<IntPtr>();
        private Spread<string> FPrevFormatFile = new Spread<string>();
        private WorkerThread FWorkerThread = new WorkerThread();
        private Spread<int> FPrevRenderFrameIdxCount = new Spread<int>();
        private Spread<ISpread<string>> FPrevFrameRendered = new Spread<ISpread<string>>();
        private Spread<bool> FIsReady = new Spread<bool>();
        private Spread<ISpread<string>> FPrevFrames = new Spread<ISpread<string>>();
        private Spread<int> FPrevRenderFrames = new Spread<int>();

        private ISpread<Dictionary<IntPtr, DX11Resource<DX11Texture2D>>> FSharedTextureCache =
            new Spread<Dictionary<IntPtr, DX11Resource<DX11Texture2D>>>();

        private int FPrevNumSpreads = 0;
        private bool FRefreshPlayer = false;
        private bool FRefreshTextures = false;
        #endregion

        #region vvvv node life cycle
        static DX11PlayerNode()
        {
            SetupDLLDirectory();
        }

        static string ResolvePluginFolderFromArchitecture()
        {
            var check64Bit = (IntPtr)0;
            if (System.Runtime.InteropServices.Marshal.SizeOf(check64Bit) == sizeof(Int64))
            {
                return "x64\\";
            }
            return "x86\\";
        }

        static void SetupDLLDirectory()
        {
            try
            {
                string rootDir = Assembly.GetExecutingAssembly().CodeBase;
                UriBuilder rootDirURI = new UriBuilder(rootDir);
                string path = Uri.UnescapeDataString(rootDirURI.Path);
                string pluginfolder = Path.GetDirectoryName(path);
                SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_APPLICATION_DIR |
                                         LOAD_LIBRARY_SEARCH_USER_DIRS |
                                         LOAD_LIBRARY_SEARCH_DEFAULT_DIRS |
                                         LOAD_LIBRARY_SEARCH_SYSTEM32);
                pluginfolder = pluginfolder + "\\" + ResolvePluginFolderFromArchitecture();
                AddDllDirectory(pluginfolder);
                OutputDebugString("Adding " + pluginfolder + " to DLL search path");
                AddDllDirectory(pluginfolder);
            }
            catch (Exception e)
            {
                OutputDebugString("Error adding DLL search path: " + e.StackTrace);
            }
        }

        public void OnImportsSatisfied()
        {
            var assembly = System.Reflection.Assembly.GetExecutingAssembly();
            var version = (AssemblyInformationalVersionAttribute)assembly
                .GetCustomAttribute(typeof(AssemblyInformationalVersionAttribute));
            var versionString = version.InformationalVersion;
            FVersion[0] = versionString;
            try
            {
                NativeInterface.DX11Player_Available();
            }
            catch (DllNotFoundException e)
            {
                FStatusOut[0] = e.Message;
            }
        }

        public void Evaluate(int spreadMax)
        {
            try
            {
                var numSpreads = Math.Max(FInFormatFile.SliceCount, 1);
                if (FPrevNumSpreads != numSpreads || FRefreshPlayer)
                {
                    FLogger.Log(LogType.Message, "Evaluate: Setting spreads to " + numSpreads);
                    FRefreshTextures = true;
                    FDX11NativePlayer.SliceCount = numSpreads;
                    FStatusOut.SliceCount = numSpreads;
                    FTextureOut.SliceCount = numSpreads;
                    FUploadSizeOut.SliceCount = numSpreads;
                    FWaitSizeOut.SliceCount = numSpreads;
                    FRenderSizeOut.SliceCount = numSpreads;
                    FPresentSizeOut.SliceCount = numSpreads;
                    FDroppedFramesOut.SliceCount = numSpreads;
                    FAvgLoadDurationMsOut.SliceCount = numSpreads;
                    FIsReady.SliceCount = numSpreads;
                    FGotFirstFrameOut.SliceCount = numSpreads;
                    FPrevFrames.SliceCount = numSpreads;
                    FSharedTextureCache.SliceCount = numSpreads;
                    FPrevRenderFrames.SliceCount = numSpreads;
                    FLoadFrameOut.SliceCount = numSpreads;
                    FRenderFrameOut.SliceCount = numSpreads;
                    FSizeOut.SliceCount = numSpreads;
                    FFormatOut.SliceCount = numSpreads;
                    FPrevFormatFile.SliceCount = numSpreads;
                    FPrevFrameRendered.SliceCount = numSpreads;
                    FPrevRenderFrameIdxCount.SliceCount = numSpreads;
                    FGotFirstFrameOut.SliceCount = numSpreads;
                    for (var i = 0; i < FDX11NativePlayer.SliceCount; i++)
                    {
                        FPrevFrames[i] = new Spread<string>();
                        FPrevFrameRendered[i] = new Spread<string>();
                        OutputDebugString("Evaluate: Destroying player - number of spreads changed");
                        DestroyPlayer(i);
                        FSharedTextureCache[i] = new Dictionary<IntPtr, DX11Resource<DX11Texture2D>>();
                        FIsReady[i] = false;
                    }
                    FPrevNumSpreads = numSpreads;
                    FRefreshPlayer = false;
                }
            }
            catch (Exception e)
            {
                FLogger.Log(LogType.Error, e.ToString());
            }

            for (var i = 0; i < FDX11NativePlayer.SliceCount; i++)
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
                        FAvgLoadDurationMsOut[i] =
                            (int)(((double)FAvgLoadDurationMsOut[i]) * 0.9 +
                                   ((double)NativeInterface.DX11Player_GetAvgLoadDurationMs(FDX11NativePlayer[i])) *
                                   0.1);
                    }

                    FStatusOut[i] = NativeInterface.DX11Player_GetStatusMessage(FDX11NativePlayer[i]);
                }
                else
                {
                    FStatusOut[i] = "Not set";
                }
            }

            // Check for changes in the format files 
            ReactOnFormatFileChanges(); 

            if (!FInFormatFile.Any())
            {
                for (var i = 0; i < FPrevFormatFile.Count(); i++)
                {
                    FPrevFormatFile[i] = null;
                }
                FPrevFormatFile.SliceCount = 0;
            }

            if (FAlwaysShowLastIn.IsChanged)
            {
                for (var i = 0; i < FDX11NativePlayer.SliceCount; i++)
                {
                    if (FDX11NativePlayer[i] != IntPtr.Zero)
                    {
                        NativeInterface.DX11Player_SetAlwaysShowLastFrame(FDX11NativePlayer[i], FAlwaysShowLastIn[i]);
                    }
                }
            }

            for (var i = 0; i < FNextFrameRenderIn.SliceCount; i++)
            {
                if (FNextFrameRenderIn[i].Count() == FPrevRenderFrameIdxCount[i]) continue;
                CreateTextureOut(i);
                FPrevRenderFrameIdxCount[i] = FNextFrameRenderIn[i].Count();
            }
        }

        public void ReactOnFormatFileChanges()
        {
            for (var i = 0; i < FInFormatFile.Count(); i++)
            {
                var availablePrevFormatFile = FPrevFormatFile.SliceCount > i;
                var availableFormatFile     = FInFormatFile.SliceCount   > i;
                if (!availableFormatFile || !availablePrevFormatFile)
                {
                    FRefreshPlayer = true;
                    continue;
                }
                var previousFormatFile = string.IsNullOrEmpty(FPrevFormatFile[i]) ? "" : FPrevFormatFile[i];
                var currentFormatFile  = string.IsNullOrEmpty(FInFormatFile[i])   ? "" : FInFormatFile[i];
                if (previousFormatFile != currentFormatFile)
                {
                    var validFormatFile = !string.IsNullOrEmpty(currentFormatFile);
                    var validPrevFormatFile = !string.IsNullOrEmpty(previousFormatFile);
                    if (validFormatFile && validPrevFormatFile)
                    {
                        var formatsAreTheSame =
                            NativeInterface.DX11Player_IsSameFormat(currentFormatFile, previousFormatFile);
                        FLogger.Log(LogType.Message,
                            "Formats are the same (DX11PlayerNative.dll): " + formatsAreTheSame);
                        if (!formatsAreTheSame)
                        {
                            DestroyPlayer(i);
                        }
                    }
                    // Update the previous format file with the current one
                    FPrevFormatFile[i] = currentFormatFile;
                }
            }
        }

        public void Dispose()
        {
            OutputDebugString("Dispose: Disposing DX11Player plugin");
            for (var i = 0; i < FDX11NativePlayer.SliceCount; i++)
            {
                if (FDX11NativePlayer[i] == IntPtr.Zero) continue;
                var player = FDX11NativePlayer[i];
                NativeInterface.DX11Player_Destroy(player);
                FDX11NativePlayer[i] = IntPtr.Zero;
                foreach (var texture in FSharedTextureCache[i].ToList())
                {
                    texture.Value.Dispose();
                    FSharedTextureCache[i].Remove(texture.Key);
                }

                this.FIsReady[i] = false;
                this.FGotFirstFrameOut[i] = false;
            }
        }

        #endregion

        #region Helper methods

        private void CreateTextureOut(int i)
        {
            FTextureOut[i] = new Spread<DX11Resource<DX11Texture2D>>();
            var numSpreads = Math.Max(FNextFrameRenderIn[i].SliceCount, 1);
            FTextureOut[i].SliceCount = numSpreads;
            FPrevFrameRendered[i].SliceCount = numSpreads;
            FPrevRenderFrameIdxCount[i] = numSpreads;
            for (var j = 0; j < numSpreads; j++)
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
                        tex.Value?.Dispose();
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
            FPrevFrames[i].SliceCount = FInTextureFiles[i].SliceCount;
            for (var j = 0; j < FPrevFrames[i].Count(); j++)
            {
                FPrevFrames[i][j] = "";
            }

            FRefreshTextures = true;
            FRefreshPlayer = false;
        }

        #endregion

        #region DX11 

        public void Update(DX11RenderContext context)
        {
            if (FRefreshTextures)
            {
                FRefreshTextures = false;
                try
                {
                    for (var i = 0; i < FDX11NativePlayer.SliceCount; i++)
                    {
                        if (FDX11NativePlayer[i] != IntPtr.Zero) continue;
                        var nativePlayer = NativeInterface.DX11Player_Create(FInFormatFile[i], FInTextureFiles[i].Count());
                        if (nativePlayer != IntPtr.Zero)
                        {
                            //NativeInterface.DX11Player_SetAlwaysShowLastFrame(nativePlayer, FAlwaysShowLastIn[i]);
                            FDX11NativePlayer[i] = nativePlayer;
                        }
                        else
                        {
                            throw new Exception("Creating native player returned nullptr");
                        }
                    }
                }
                catch (Exception ex)
                {
                    FLogger.Log(ex);
                }
            }

            for (var i = 0; i < FDX11NativePlayer.SliceCount; i++)
            {
                if (FDX11NativePlayer[i] == IntPtr.Zero) continue;
                if (!FIsReady[i] && NativeInterface.DX11Player_IsReady(FDX11NativePlayer[i]))
                {
                    FIsReady[i] = true;
                }

                var nextFile = "";
                if (FInTextureFiles[i].Any())
                {
                    for (var j = 0; j < FNextFrameRenderIn[i].Count(); j++)
                    {
                        try
                        {
                            var nextFrameRenderIdx = FNextFrameRenderIn[i][j];
                            var prevFrameRender = FPrevFrameRendered[i][j];
                            var renderFrame = (Math.Max(0, nextFrameRenderIdx) % FInTextureFiles[i].Count());
                            nextFile = FInTextureFiles[i][renderFrame];
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
                        catch (Exception e)
                        {
                            FLogger.Log(LogType.Error, e.StackTrace);
                        }
                    }
                }

                try
                {
                    FRenderFrameOut[i] = NativeInterface.DX11Player_GetCurrentRenderFrame(FDX11NativePlayer[i]);
                    FGotRequestedFrameOut[i] = nextFile == FRenderFrameOut[i];
                    NativeInterface.DX11Player_Update(FDX11NativePlayer[i]);
                }
                catch (Exception e)
                {
                    FLogger.Log(LogType.Warning, e.Message);
                }

                if (!this.FIsReady[i]) continue;
                NativeInterface.DX11Player_SetSystemFrames(FDX11NativePlayer[i], FInTextureFiles[i].ToArray(),
                    FInTextureFiles[i].Count());
                var diff = FInTextureFiles[i].Except(FPrevFrames[i]);
                foreach (var f in diff)
                {
                    NativeInterface.DX11Player_SendNextFrameToLoad(FDX11NativePlayer[i], f);
                }

                if (FDX11NativePlayer[i] != IntPtr.Zero && FPrevFrames[i].Count() != FInTextureFiles[i].Count())
                {
                    FRefreshPlayer = true;
                    OutputDebugString("Destroying player number of files to load has changed");
                }

                FPrevFrames[i] = FInTextureFiles[i].Clone();
                FLoadFrameOut[i] = NativeInterface.DX11Player_GetCurrentLoadFrame(FDX11NativePlayer[i]);
            }
        }

        public void Destroy(DX11RenderContext context, bool force)
        {
            for (var i = 0; i < FDX11NativePlayer.SliceCount; i++)
            {
                foreach (var handle in this.FSharedTextureCache[i].ToList())
                {
                    if (!handle.Value.Contains(context)) continue;
                    handle.Value.Dispose();
                    FSharedTextureCache[i].Remove(handle.Key);
                }
                for (var j = 0; j < FTextureOut[i].Count(); j++)
                {
                    if (this.FTextureOut[i][j].Contains(context))
                    {
                        this.FTextureOut[i][j] = null;
                    }
                }
            }
        }

        #endregion
    } 
}
