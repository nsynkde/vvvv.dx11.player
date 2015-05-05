#region usings
using System;
using System.ComponentModel.Composition;
using System.Runtime.InteropServices;

using VVVV.Core.Logging;
using VVVV.PluginInterfaces.V1;
using VVVV.PluginInterfaces.V2;

#endregion usings

namespace VVVV.Nodes.Bluefish
{
    
    #region PluginInfo
    [PluginInfo(Name = "SDIOut",
                Category = "Bluefish",
                Help = "Represents an SDIOut of a Bluefish device, each memory channel can be connected to one or more SDIOuts",
                Tags = "",
                Author = "nsynk",
                AutoEvaluate = true)]
    #endregion PluginInfo
	public class SDIOut : IPluginEvaluate, IPartImportsSatisfiedNotification, IDisposable
    {

        #region fields & pins
#pragma warning disable 0649
        [Input("Device")]
        IDiffSpread<uint> FInDevice;

		[Input("SDIOut 0 Memory Channel")]
        IDiffSpread<uint> FInSDIOut0;

        [Input("SDIOut 1 Memory Channel")]
        IDiffSpread<uint> FInSDIOut1;

        [Input("SDIOut 2 Memory Channel")]
        IDiffSpread<uint> FInSDIOut2;

        [Input("SDIOut 3 Memory Channel")]
        IDiffSpread<uint> FInSDIOut3;

        /*[Input("Transport use B on 3")]
        IDiffSpread<bool> FInTransport;*/

		[Output("Status SDIOut 0")]
        ISpread<string> FOutStatus0;

        [Output("Status SDIOut 1")]
        ISpread<string> FOutStatus1;

        [Output("Status SDIOut 2")]
        ISpread<string> FOutStatus2;

        [Output("Status SDIOut 3")]
        ISpread<string> FOutStatus3;

        [Input("Sync", DefaultValue = 1)]
        IDiffSpread<bool> FInSyncLoop;

        [Import]
        ILogger FLogger;

		/*[Import]
		IHDEHost FHDEHost;*/

#pragma warning restore
        #endregion fields & pins

        bool FFirstRun = true;
        BlueSDIOut[] FInstances = new BlueSDIOut[4];
        IDiffSpread<uint>[] FInSDIOutRef = new IDiffSpread<uint>[4];
        ISpread<string>[] FOutStatusRef = new ISpread<string>[4];

        [ImportingConstructor]
		public SDIOut(IPluginHost host)
        {
            
        }

        public void Evaluate(int SpreadMax)
        {
            if (FFirstRun)
            {

                FInSDIOutRef[0] = FInSDIOut0;
                FInSDIOutRef[1] = FInSDIOut1;
                FInSDIOutRef[2] = FInSDIOut2;
                FInSDIOutRef[3] = FInSDIOut3;


                FOutStatusRef[0] = FOutStatus0;
                FOutStatusRef[1] = FOutStatus1;
                FOutStatusRef[2] = FOutStatus2;
                FOutStatusRef[3] = FOutStatus3;

                for (int i = 0; i < 4; i++)
                {
                    try
                    {
                        FOutStatusRef[i].SliceCount = 1;
                        FInSDIOutRef[i].SliceCount = 1;
                        BlueSDIOut inst = new BlueSDIOut((int)FInDevice[0] + 1, i);

                        FInstances[i] = inst;
                        FOutStatusRef[i][0] = "OK";
                    }
                    catch (Exception e)
                    {
                        FInstances[i] = null;
                        FOutStatusRef[i][0] = e.ToString();
                    }
                }


                FFirstRun = false;
            }
            if (FFirstRun || FInSDIOut0.IsChanged || FInSDIOut1.IsChanged || FInSDIOut2.IsChanged || FInSDIOut3.IsChanged)
			{
                for (int i = 0; i < 4; i++)
                {
                    FLogger.Log(LogType.Message, "Routing " + FInSDIOutRef[i][0]  + " to " + i);
                    FInstances[i].Route((int)FInSDIOutRef[i][0]);
                    FFirstRun = true;
                }
			}
            if (FInSyncLoop.IsChanged)
            {
                if(FInSyncLoop[0]){
                    BlueSDIOutSwapChain.EnableSync((int)FInDevice[0] + 1);
                }
                else
                {
                    BlueSDIOutSwapChain.DisableSync((int)FInDevice[0] + 1);
                }
            }
        }

        public void OnImportsSatisfied()
        {
        }


		public void Dispose()
		{
			/*foreach (var slice in FInstances)
				if (slice != null)
					slice.Dispose();*/
			GC.SuppressFinalize(this);
		}

        internal class BlueSDIOutSwapChain
        {
            [DllImport("BluePlayback.dll", SetLastError = false)]
            internal static extern void EnableSync(int device);

            [DllImport("BluePlayback.dll", SetLastError = false)]
            internal static extern void DisableSync(int device);
        }
	}
}
