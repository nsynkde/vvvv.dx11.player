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

namespace VVVV.Nodes.Bluefishs
{
	#region PluginInfo
    [PluginInfo(Name = "DeviceAttributes", Category = "Bluefish", Help = "Report attributes of a Bluefish device", Tags = "", Author = "elliotwoods", AutoEvaluate = true)]
	#endregion PluginInfo
	public class DeviceAttributesNode : IPluginEvaluate
	{
		#region fields & pins
		[Input("Device")]
        IDiffSpread<BluePlaybackNet> FInput;

		[Output("Serial port supported")]
		ISpread<bool> FSerialPort;

		[Output("Serial port name")]
		ISpread<string> FSerialPortName;

		[Output("Status")]
		ISpread<string> FPinOutStatus;

		[Import]
		ILogger FLogger;

		#endregion fields & pins

		[ImportingConstructor]
		public DeviceAttributesNode(IPluginHost host)
		{

		}

		//called when data for any output pin is requested
		public void Evaluate(int SpreadMax)
		{
			if (FInput.IsChanged)
			{
				Refresh();
			}
		}

		private void Refresh()
		{
			FSerialPort.SliceCount = FInput.SliceCount;
			FSerialPortName.SliceCount = FInput.SliceCount;
			FPinOutStatus.SliceCount = FInput.SliceCount;

			for (int i = 0; i < FInput.SliceCount; i++)
			{
				try
				{
					BluePlaybackNet device = FInput[i];
					
                    //figure device attributes


					FPinOutStatus[i] = "Not implemented (no reference material available)";
				}
				catch (Exception e)
				{
					FPinOutStatus[i] = e.Message;
				}
			}
		}
	}
}
