#region usings
using System;
using System.ComponentModel.Composition;
using System.Drawing;
using System.Threading;

using VVVV.PluginInterfaces.V1;
using VVVV.PluginInterfaces.V2;
using VVVV.Utils.VMath;
using VVVV.Core.Logging;
using BluePlaybackNetLib;

#endregion usings

namespace VVVV.Nodes.Bluefish
{
	#region PluginInfo
	[PluginInfo(
        Name = "About",
        Category = "Bluefish",
        Help = "Report version details of Bluefish (yet do be done)",
        Tags = "",
        Author = "nsynk",
        AutoEvaluate = true)]
	#endregion PluginInfo
	public class AboutNode : IPluginEvaluate
	{
		#region fields & pins
		[Output("Bluefish Velvet Version")]
		ISpread<string> FPinOutVelvetVersion;

		[Output("Status")]
		ISpread<string> FPinOutStatus;

		[Import]
		ILogger FLogger;

		#endregion fields & pins

		[ImportingConstructor]
		public AboutNode(IPluginHost host)
		{

		}

		bool FFirstRun = true;
		//called when data for any output pin is requested
		public void Evaluate(int SpreadMax)
		{
			if (FFirstRun)
			{
                FLogger.Log(LogType.Message, "Bluefish: About: first run");

				FFirstRun = false;
				Refresh();
			}
		}

		private void Refresh()
		{
			try
			{
                FLogger.Log(LogType.Message, "Bluefish: About: create blueplayback net");
                BluePlaybackNet blueVelvet = new BluePlaybackNet();

                FLogger.Log(LogType.Message, "Bluefish: About: create device");
                // create bluefish device
                blueVelvet.BluePlaybackInterfaceCreate();

                
                // get device count
                FLogger.Log(LogType.Message, "Bluefish: About: get device count");
                int count = blueVelvet.GetDevicecount();

                FLogger.Log(LogType.Message, "Bluefish: About: device count: " + count);


                FLogger.Log(LogType.Message, "Bluefish: About: configure device");
                int videoMode = (int)BlueFish_h.EVideoMode.VID_FMT_PAL;

                //MEM_FMT_ARGB_PC=6
                //MEM_FMT_BGR=9
                int memFormat = (int)BlueFish_h.EMemoryFormat.MEM_FMT_BGR;

                int updateMethod = (int)BlueFish_h.EUpdateMethod.UPD_FMT_FRAME;


                
                /*

                // configure card
                blueVelvet.BluePlaybackInterfaceConfig(1, // Device Number
                                                            0, // output channel
                                                            videoMode, // video mode //VID_FMT_PAL=0
                                                            memFormat, // memory format //MEM_FMT_ARGB_PC=6 //MEM_FMT_BGR=9
                                                            updateMethod, // update type (frame, field) //UPD_FMT_FRAME=1
                                                            0, // video destination
                                                            0, // audio destination
                                                            0  // audio channel mask
                                                            ); //Dev 1, Output channel A, PAL, BGRA, FRAME_MODE, not used, not used, not used



                /*
                // get serial number
                FLogger.Log(LogType.Message, "Bluefish: About: get device serial number");
                blueVelvet.BluePlaybackGetSerialNumber();

                //FLogger.Log(LogType.Message, "Bluefish: About: device serial number: " + sn);


                /*
                FLogger.Log(LogType.Message, "get version");
                string velvetVersion = blueVelvet.BlueVelvetVersion();
			
                FPinOutVelvetVersion[0] = velvetVersion;
				
				FPinOutStatus[0] = "OK";
                */
			}

			catch (Exception e)
			{
				FPinOutStatus[0] = "ERROR : " + e.Message;
			}

		}

	}
}
