﻿#region usings
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

        [Input("TextureHandleDX11")]
        IDiffSpread<ulong> FInHandleDX11;

        [Input("TextureHandleDX9")]
        IDiffSpread<uint> FInHandleDX9;

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
        bool FFirstRun = true;
        #endregion fields & pins
        bool DX11ChangedLast;

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

        [DllImport("kernel32.dll", CharSet = CharSet.Auto)]
        public static extern void OutputDebugString(string message);

        public void Evaluate(int SpreadMax)
        {
            if(FInHandleDX11.IsChanged)
            {
                DX11ChangedLast = true;
            }
            else if (FInHandleDX9.IsChanged)
            {
                DX11ChangedLast = false;
            }

            if (FFirstRun || FInDevice.IsChanged || FInMode.IsChanged || FInFormat.IsChanged || FInHandleDX11.IsChanged || FInHandleDX9.IsChanged || FInEnabled.IsChanged || FInOutChannel.IsChanged)
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

                        var handle = (DX11ChangedLast && FInHandleDX11[i] != 0) ? FInHandleDX11[i] : FInHandleDX9[i];

                        BluefishSource inst = new BluefishSource(FInDevice[i], FInOutChannel[i], FInMode[i], FInFormat[i], handle, FLogger);

                        FInstances.Add(inst);
                        FOutStatus[i] = "OK";
                        FOutSerialNumber[i] = inst.SerialNumber;
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
                    //FOutDroppedFrames[i] = FInstances[i].DroppedFrames;
                    FOutAvgFrameTime[i] = FInstances[i].AvgDuration;
                    FOutMaxFrameTime[i] = FInstances[i].MaxDuration;
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

                instance.OnPresent();
                if (this.FInSyncLoop[i])
                {
                    instance.WaitSync();
                }
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
