using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.ComponentModel.Composition;
using VVVV.Core.Logging;
using BluePlaybackNetLib;

namespace VVVV.Nodes.Bluefish
{
	class DeviceRegister
	{
		static public DeviceRegister Singleton = null;

        static public void SetupSingleton(ILogger FLogger){
            if (Singleton == null)
            {
                Singleton = new DeviceRegister(FLogger);
            }
        }

        ILogger FLogger;

        DeviceRegister(ILogger FLogger)
        {
            this.FLogger = FLogger;
        }

		public class DeviceIndex
		{
			public DeviceIndex(int Index)
			{
				this.Index = Index;
			}

			public int Index;
		}

		public class Device
		{
			public BluePlaybackNet BluePlayback;
			public string SerialNumber;
		}

		List<Device> FDevices = new List<Device>();
		public List<Device> Devices
		{
			get
			{
				return FDevices;
			}
		}

        public int FDeviceCount = 0;
		public int Count
		{
			get
            {
                if (FDevices.Count == 0)
                {
                    Refresh();
                }
				int result = 0;
				WorkerThread.Singleton.PerformBlocking(() =>
					{
						result = FDevices.Count;
					});
				return result;
			}
		}

		public string GetSerialNumber(int index)
		{
			string result = "";
			WorkerThread.Singleton.PerformBlocking(() =>
			{
				result = FDevices[index].SerialNumber;
			});
			return result;
		}

        public BluePlaybackNet GetDeviceHandle(int index)
		{
			if (this.Count == 0)
			{
				Refresh();
			}
			if (this.Count <= index)
				throw (new Exception("No Bluefish device available"));
			else
                return FDevices[index].BluePlayback;
		}

		public void Refresh()
		{
            FLogger.Log(LogType.Message, "Bluefish: Device register: Refresh()");

            WorkerThread.Singleton.PerformBlocking(() => {

                FLogger.Log(LogType.Message, "Bluefish: Device register: WorkerThread: clear old devices");

                foreach (var oldDevice in FDevices)
                {
                    Marshal.ReleaseComObject(oldDevice.BluePlayback);
                }
                FDevices.Clear();



                // one devie only for now
                // do more later...
                FLogger.Log(LogType.Message, "Bluefish: Device register: create device");
                BluePlaybackNet device = new BluePlaybackNet();
                // create bluefish device
                device.BluePlaybackInterfaceCreate();


                // get device count
                FLogger.Log(LogType.Message, "Bluefish: Device register: get device count");
                FDeviceCount = device.GetDevicecount();

                FLogger.Log(LogType.Message, "Bluefish: Device register: device count: " + FDeviceCount);


                // add device, with default names...
                FDevices.Add(new Device()
                {
                    BluePlayback = device,
                    SerialNumber = ""
                });
                //device.BluePlaybackGetSerialNumber(),//this hangs the program

            });
        }
	}
}
