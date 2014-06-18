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

        [Output("Serial Number")]
		ISpread<string> FPinOutSerialNumber;

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
                FPinOutSerialNumber.SliceCount = 0;

                FLogger.Log(LogType.Message, "Bluefish: ListDevice: get DeviceRegister singleton");
                DeviceRegister.SetupSingleton(FLogger);
                var register = DeviceRegister.Singleton;

                FLogger.Log(LogType.Message, "Bluefish: ListDevice: got DeviceRegister. call refresh");
                if (register != null)
                {
                    register.Refresh();
                }
                else
                {
                    FLogger.Log(LogType.Message, "Couldn't get register singleton");
                }


                FLogger.Log(LogType.Message, "Bluefish: ListDevice: after refresh");
				for (int i = 0; i < register.Count; i++)
				{
                    FLogger.Log(LogType.Message, "Bluefish: ListDevice: interate: " + i);

					FPinOutDevices.Add(new DeviceRegister.DeviceIndex(i));
                    FPinOutSerialNumber.Add(register.GetSerialNumber(i));
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
