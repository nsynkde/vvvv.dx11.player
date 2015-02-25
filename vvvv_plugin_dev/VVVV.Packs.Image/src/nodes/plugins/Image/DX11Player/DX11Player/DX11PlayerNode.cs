﻿using System;
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

namespace VVVV.Nodes.DX11PlayerNode
{
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
                AddDllDirectory(pluginfolder);
            }catch(Exception){
            }

        }

#pragma warning disable 0649
        [Input("Directory", StringType = StringType.Directory)]
        public IDiffSpread<string> FDirectoryIn;

        [Input("Filemask", DefaultString = "*")]
        public IDiffSpread<string> FFilemaskIn;

        [Input("fps", DefaultValue = 60)]
        public IDiffSpread<int> FFPSIn;

        [Input("internal rate enabled", DefaultValue = 1)]
        public IDiffSpread<bool> FInternalRate;

        [Input("next frame")]
        public IDiffSpread<int> FNextFrameIn;


        [Output("texture handle")]
        public ISpread<UInt64> FTexHandleOut;

        [Output("status")]
        public ISpread<string> FStatusOut;

        [Output("Texture")]
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

        private Spread<IntPtr> FDX11NativePlayer = new Spread<IntPtr>();

        [Import]
        ILogger FLogger;

#pragma warning restore

        private bool FFirstRun = true;
        private bool FRefreshTextures = false;

        public void Evaluate(int spreadMax)
        {
            try
            {
                if (FFirstRun)
                {
                    FFirstRun = false;
                    FRefreshTextures = true;
                    FTexHandleOut.SliceCount = spreadMax;
                    FDX11NativePlayer.SliceCount = spreadMax;
                    FStatusOut.SliceCount = spreadMax;
                    FTextureOut.SliceCount = spreadMax;;
                    FUploadSizeOut.SliceCount = spreadMax;
                    FWaitSizeOut.SliceCount = spreadMax;
                    FRenderSizeOut.SliceCount = spreadMax;
                    FDroppedFramesOut.SliceCount = spreadMax;
                    FLoadFrameOut.SliceCount = spreadMax;
                    FRenderFrameOut.SliceCount = spreadMax;
                    FAvgLoadDurationMsOut.SliceCount = spreadMax;
                    for (int i = 0; i < FDX11NativePlayer.SliceCount; i++)
                    {
                        FDX11NativePlayer[i] = IntPtr.Zero;
                    }
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
            }

            if (FDirectoryIn.IsChanged)
            {
                for (int i = 0; i < FDX11NativePlayer.SliceCount; i++)
                {
                    if (FDX11NativePlayer[i] != IntPtr.Zero)
                    {
                        if (NativeInterface.DX11Player_DirectoryHasChanged(FDX11NativePlayer[i], FDirectoryIn[i]) == 1)
                        {
                            FDX11NativePlayer[i] = IntPtr.Zero;
                            foreach (var tex in this.FTextureOut[i].Data.Values)
                            {
                                tex.Dispose();
                            }
                            this.FTextureOut[i].Data.Clear();
                            FRefreshTextures = true;
                        }
                    }
                }
            }

            if (FNextFrameIn.IsChanged)
            {
                for (int i = 0; i < FDX11NativePlayer.SliceCount; i++)
                {
                    if (FDX11NativePlayer[i] != IntPtr.Zero)
                    {
                        NativeInterface.DX11Player_SendNextFrameToLoad(FDX11NativePlayer[i], FNextFrameIn[i]);
                    }
                }
            }

            if (FInternalRate.IsChanged)
            {
                for (int i = 0; i < FDX11NativePlayer.SliceCount; i++)
                {
                    if (FDX11NativePlayer[i] != IntPtr.Zero)
                    {
                        NativeInterface.DX11Player_SetInternalRate(FDX11NativePlayer[i], FInternalRate[i] ? 1 : 0);
                    }
                }
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
                            FLogger.Log(LogType.Message, "Creating " + FDirectoryIn[i]);
                            var nativePlayer = NativeInterface.DX11Player_Create(FDirectoryIn[i]);
                            if (nativePlayer != IntPtr.Zero)
                            {
                                NativeInterface.DX11Player_SetFPS(nativePlayer, FFPSIn[i]);
                                FDX11NativePlayer[i] = nativePlayer;
                                //FTexHandleOut[i] = (UInt64)NativeInterface.DX11Player_GetSharedHandle(FDX11NativePlayer[i]);
                                FTextureOut[i] = new DX11Resource<DX11Texture2D>();
                            }
                            else
                            {
                                throw new Exception("Creating native player returned nullptr");
                            }
                        }
                        //Texture2D tex = Texture2D.FromPointer(NativeInterface.DX11Player_GetTexturePointer(FDX11NativePlayer[i]));
                        Texture2D tex = context.Device.OpenSharedResource<Texture2D>(NativeInterface.DX11Player_GetSharedHandle(FDX11NativePlayer[i]));

                        ShaderResourceView srv = new ShaderResourceView(context.Device, tex);

                        DX11Texture2D resource = DX11Texture2D.FromTextureAndSRV(context, tex, srv);

                        this.FTextureOut[i][context] = resource;
                    }
                }
                catch (Exception ex)
                {
                    FLogger.Log(ex);
                }
            }

            for (int i = 0; i < FDX11NativePlayer.SliceCount; i++)
            {
                if (!this.FTextureOut[i].Data.ContainsKey(context))
                {
                    FRefreshTextures = true;
                }
                if (FDX11NativePlayer[i] != IntPtr.Zero)
                {
                    NativeInterface.DX11Player_OnRender(FDX11NativePlayer[i]);
                }
                /*Texture2D tex = context.Device.OpenSharedResource<Texture2D>(NativeInterface.DX11Player_GetSharedHandle(FDX11NativePlayer[i]));

                ShaderResourceView srv = new ShaderResourceView(context.Device, tex);

                DX11Texture2D resource = DX11Texture2D.FromTextureAndSRV(context, tex, srv);

                this.FTextureOut[i][context] = resource;*/
            }
        }

        public void Destroy(IPluginIO pin, DX11RenderContext context, bool force)
        {
            for (int i = 0; i < FDX11NativePlayer.SliceCount; i++)
            {
                if (FDX11NativePlayer[i] != IntPtr.Zero)
                {
                    this.FTextureOut[i].Dispose(context);
                }
            }
        }
    }


    internal class NativeInterface
    {
        [DllImport("Native.dll", SetLastError = false, CharSet = CharSet.Ansi)]
        internal static extern IntPtr DX11Player_Create(string directory);

        [DllImport("Native.dll", SetLastError = false)]
        internal static extern void DX11Player_Destroy(IntPtr player);

        [DllImport("Native.dll", SetLastError = false)]
        internal static extern void DX11Player_OnRender(IntPtr player);

        [DllImport("Native.dll", SetLastError = false)]
        internal static extern IntPtr DX11Player_GetSharedHandle(IntPtr player);
        
        [DllImport("Native.dll", SetLastError = false)]
        internal static extern void DX11Player_DoneRender(IntPtr player);

        [DllImport("Native.dll", SetLastError = false, CharSet = CharSet.Ansi)]
        internal static extern string DX11Player_GetDirectory(IntPtr player);
        [DllImport("Native.dll", SetLastError = false, CharSet = CharSet.Ansi)]
        internal static extern int DX11Player_DirectoryHasChanged(IntPtr player, string dir);
        [DllImport("Native.dll", SetLastError = false)]
        internal static extern int DX11Player_GetUploadBufferSize(IntPtr player);
        [DllImport("Native.dll", SetLastError = false)]
        internal static extern int DX11Player_GetWaitBufferSize(IntPtr player);
        [DllImport("Native.dll", SetLastError = false)]
        internal static extern int DX11Player_GetRenderBufferSize(IntPtr player);
        [DllImport("Native.dll", SetLastError = false)]
        internal static extern int DX11Player_GetDroppedFrames(IntPtr player);
        [DllImport("Native.dll", SetLastError = false)]
        internal static extern int DX11Player_GetCurrentLoadFrame(IntPtr player);
        [DllImport("Native.dll", SetLastError = false)]
        internal static extern int DX11Player_GetCurrentRenderFrame(IntPtr player);
        [DllImport("Native.dll", SetLastError = false)]
        internal static extern int DX11Player_GetAvgLoadDurationMs(IntPtr player);
        [DllImport("Native.dll", SetLastError = false)]
        internal static extern void DX11Player_SetFPS(IntPtr player,int fps);
        [DllImport("Native.dll", SetLastError = false)]
        internal static extern void DX11Player_SendNextFrameToLoad(IntPtr player, int nextFrame);
        [DllImport("Native.dll", SetLastError = false)]
        internal static extern void DX11Player_SetInternalRate(IntPtr player, int enabled);
    }

}