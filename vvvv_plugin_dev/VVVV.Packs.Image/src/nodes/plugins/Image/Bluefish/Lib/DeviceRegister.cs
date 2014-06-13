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
			public string ModelName;
			public string DisplayName;
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
				int result = 0;
				WorkerThread.Singleton.PerformBlocking(() =>
					{
						result = FDevices.Count;
					});
				return result;
			}
		}

		public string GetModelName(int index)
		{
			string result = "";
			WorkerThread.Singleton.PerformBlocking(() =>
			{
				result = FDevices[index].ModelName;
			});
			return result;
		}

		public string GetDisplayName(int index)
		{
			string result = "";
			WorkerThread.Singleton.PerformBlocking(() =>
			{
				result = FDevices[index].DisplayName;
			});
			return result;
		}

        public BluePlaybackNet GetDeviceHandle(int index)
		{
			if (this.Count == 0)
			{
				Refresh();
			}
			if (this.Count == 0)
				throw (new Exception("No Bluefish device available"));
			else
                return FDevices[index % FDevices.Count].BluePlayback;
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

                // get serial number
                //string sn = device.BluePlaybackGetSerialNumber();
                
                //device.BluePlaybackGetSerialNumber();

                // add device, with default names...
                FDevices.Add(new Device()
                {
                    BluePlayback = device,
                    ModelName = "Bluefish Model",
                    DisplayName = "sn"
                });

                /*
                var iterator = new CDeckLinkIterator();
                IDeckLink device;
                string modelName, displayName;
                while (true)
                {
                    iterator.Next(out device);
                    if (device == null)
                        break;

                    device.GetModelName(out modelName);
                    device.GetDisplayName(out displayName);

                    FDevices.Add(new Device()
                    {
                        DeviceHandle = device,
                        ModelName = modelName,
                        DisplayName = displayName
                    });
                }* */

            });
        }
	}
}
