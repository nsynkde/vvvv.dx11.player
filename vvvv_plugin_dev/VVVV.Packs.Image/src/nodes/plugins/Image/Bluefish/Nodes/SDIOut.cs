#region usings
using System;
using System.ComponentModel.Composition;
using System.Runtime.InteropServices;

using SlimDX;
using SlimDX.Direct3D9;
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
using VVVV.Nodes.Bluefish.Lib;

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

		[Input("SDIOut")]
        IDiffSpread<uint> FInSDIOut;

        [Input("Transport")]
        IDiffSpread<BlueFish_h.EHdSdiTransport> FInTransport;

        [Input("Memory Channel")]
        IDiffSpread<uint> FInMemChannel;

		[Output("Status")]
        ISpread<string> FOutStatus;

        [Import]
        ILogger FLogger;

		[Import]
		IHDEHost FHDEHost;

#pragma warning restore

        bool FFirstRun = true;
        #endregion fields & pins
        Spread<BlueSDIOut> FInstances = new Spread<BlueSDIOut>();

        [ImportingConstructor]
		public SDIOut(IPluginHost host)
        {
        }

        public void Evaluate(int SpreadMax)
        {
            if (FFirstRun || FInSDIOut.IsChanged || FInMemChannel.IsChanged )
			{
				/*foreach(var slice in FInstances)
					if (slice != null)
						slice.Dispose();*/

				FInstances.SliceCount = 0;
				FOutStatus.SliceCount = SpreadMax;

				for (int i=0; i<SpreadMax; i++)
				{
					try
					{

                        BlueSDIOut inst = new BlueSDIOut((int)FInDevice[i], (int)FInSDIOut[i]); 

						FInstances.Add(inst);
						FOutStatus[i] = "OK";
					}
					catch(Exception e)
					{
						FInstances.Add(null);
						FOutStatus[i] = e.ToString();
					}
				}

                FFirstRun = true;
			}

            if (FFirstRun || FInMemChannel.IsChanged)
            {
                for (int i = 0; i < FInstances.SliceCount; i++)
                {
                    if (FInstances[i] != null)
                        FInstances[i].Route((int)FInMemChannel[i]);
                }

            }

            if (FFirstRun || FInTransport.IsChanged)
            {
                for (int i = 0; i < FInstances.SliceCount; i++)
                {
                    if (FInstances[i] != null)
                        FInstances[i].Transport = FInTransport[i];
                }

            }


            FFirstRun = false;
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

	}
}
