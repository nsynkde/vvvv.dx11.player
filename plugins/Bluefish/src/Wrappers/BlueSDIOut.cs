using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.InteropServices;

namespace VVVV.Nodes.Bluefish
{
    class BlueSDIOut
    {
        public BlueSDIOut(int device, int sdi_out)
        {
            pBlueSDIOut = BlueSDIOutNativeInterface.BlueSDIOutCreate(device, sdi_out);
            if (pBlueSDIOut == null)
                throw new Exception("Couldn't attach to device " + device + " SDI out " + sdi_out);
        }

        public void Route(int mem_channel){
            BlueSDIOutNativeInterface.BlueSDIOutRoute(pBlueSDIOut, mem_channel);
        }

        public BlueFish_h.EHdSdiTransport Transport
        {
            set
            {
                BlueSDIOutNativeInterface.BlueSDIOutSetTransport(pBlueSDIOut, (uint)value);
            }
        }

        private IntPtr pBlueSDIOut;
    }


    internal class BlueSDIOutNativeInterface
    {
        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern IntPtr BlueSDIOutCreate(int device, int sdi_out);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern void BlueSDIOutRoute(IntPtr sdiout_handle, int mem_channel);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern void BlueSDIOutSetTransport(IntPtr sdiout_handle, uint dualLink);
    }
}
