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
        Author = "ingorandolf",
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
                FLogger.Log(LogType.Debug, "first run");

				FFirstRun = false;
				Refresh();
			}
		}

		private void Refresh()
		{
			try
			{
                FLogger.Log(LogType.Debug, "create blueplayback net");

                BluePlaybackNet blueVelvet = new BluePlaybackNet();

                FLogger.Log(LogType.Debug, "get version");

                string velvetVersion = blueVelvet.BlueVelvetVersion();
			
                FPinOutVelvetVersion[0] = velvetVersion;
				
				FPinOutStatus[0] = "OK";
			}

			catch (Exception e)
			{
				FPinOutStatus[0] = "ERROR : " + e.Message;
			}

		}

	}
}
