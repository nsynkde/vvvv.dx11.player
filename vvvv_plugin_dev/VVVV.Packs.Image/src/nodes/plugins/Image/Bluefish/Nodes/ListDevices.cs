#region usings
using System;
using System.ComponentModel.Composition;
using System.Drawing;
using System.Threading;

using VVVV.PluginInterfaces.V1;
using VVVV.PluginInterfaces.V2;
using VVVV.Utils.VMath;
using VVVV.Core.Logging;
using System.Collections.Generic;

#endregion usings

namespace VVVV.Nodes.Bluefish
{
	#region PluginInfo
    [PluginInfo(Name = "ListDevices",
        Category = "Bluefish",
        Help = "List Bluefish devices",
        Tags = "",
        Author = "nsynk",
        AutoEvaluate = true)]
	#endregion PluginInfo
	public class ListDevicesNode : IPluginEvaluate
	{
		#region fields & pins
		[Input("Refresh", IsBang = true, IsSingle = true)]
		ISpread<bool> FPinInRefresh;

		[Output("Device")]
		ISpread<DeviceRegister.DeviceIndex> FPinOutDevices;

		[Output("Model Name")]
		ISpread<string> FPinOutModelName;

		[Output("Display Name")]
		ISpread<string> FPinOutDisplayName;

		[Output("Status")]
		ISpread<string> FPinOutStatus;

		[Import]
		ILogger FLogger;

		#endregion fields & pins

		[ImportingConstructor]
		public ListDevicesNode(IPluginHost host)
		{

		}

		bool FFirstRun = true;
		//called when data for any output pin is requested
		public void Evaluate(int SpreadMax)
		{
			if (FFirstRun || FPinInRefresh[0])
			{
                FLogger.Log(LogType.Message, "Bluefish: ListDevice: first run");

				FFirstRun = false;
				Refresh();
			}
		}

		private void Refresh()
		{
			try
			{
				FPinOutDevices.SliceCount = 0;
				FPinOutModelName.SliceCount = 0;
				FPinOutDisplayName.SliceCount = 0;

                FLogger.Log(LogType.Message, "Bluefish: ListDevice: get DeviceRegister singleton");
				var register = DeviceRegister.Singleton;
				register.Refresh();

				for (int i = 0; i < register.Count; i++)
				{
                    FLogger.Log(LogType.Message, "Bluefish: ListDevice: interate: " + i);

					FPinOutDevices.Add(new DeviceRegister.DeviceIndex(i));
					FPinOutModelName.Add(register.GetModelName(i));
					FPinOutDisplayName.Add(register.GetDisplayName(i));
				}
				FPinOutStatus[0] = "OK devices: " + register.FDeviceCount;

                FLogger.Log(LogType.Message, "OK devices: " + register.FDeviceCount);
			}

			catch (Exception e)
			{
				FPinOutStatus[0] = "ERROR : " + e.Message;
                FLogger.Log(LogType.Message, "Bluefish: ListDevice: ERROR: " + e.Message);
			}

		}

	}
}
