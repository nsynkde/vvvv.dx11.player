using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

namespace BluePlaybackNetLib
{
    public class BluePlaybackNet: IDisposable
    {
        private IntPtr pBluePlayback;

        public Int32 BluePlaybackInterfaceCreate()
        {
            Int32 error_code;
            pBluePlayback = BluePlaybackNativeInterface.BluePlaybackCreate();
            error_code = 0;
            return error_code;
        }

        public Int32 BluePlaybackInterfaceConfig(Int32 inDevNo, Int32 inChannel, Int32 inVidFmt, Int32 inMemFmt, Int32 inUpdFmt, Int32 inVideoDestination, Int32 inAudioDestination, Int32 inAudioChannelMask)
        {
            Int32 error_code = 0;
            error_code = BluePlaybackNativeInterface.BluePlaybackConfig(pBluePlayback, inDevNo, inChannel, inVidFmt, inMemFmt, inUpdFmt, inVideoDestination, inAudioDestination, inAudioChannelMask);
            return error_code;
        }

        public void BluePlaybackInterfaceStart()
        {
            BluePlaybackNativeInterface.BluePlaybackStart(pBluePlayback);
        }

        public void BluePlaybackInterfaceStop()
        {
            BluePlaybackNativeInterface.BluePlaybackStart(pBluePlayback);
        }

        public int BluePlaybackInterfaceRender(IntPtr pBuffer)
        {
            return BluePlaybackNativeInterface.BluePlaybackRender(pBluePlayback, pBuffer);
        }

        public void BluePlaybackInterfaceDestroy()
        {
            BluePlaybackNativeInterface.BluePlaybackDestroy(pBluePlayback);
            pBluePlayback = IntPtr.Zero;
        }

        public int BluePlaybackSetCardProperty(int nProp, uint nValue)
        {
            return BluePlaybackNativeInterface.BluePlaybackSetCardProperty(pBluePlayback, nProp, nValue);
        }

        public int BluePlaybackSetCardPropertyInt64(int nProp, Int64 nValue)
        {
            return BluePlaybackNativeInterface.BluePlaybackSetCardPropertyInt64(pBluePlayback, nProp, nValue);
        }

        public ulong BluePlaybackGetMemorySize()
        {
            return BluePlaybackNativeInterface.BluePlaybackGetMemorySize(pBluePlayback);
        }

        public string BluePlaybackGetSerialNumber()
        {
            return BluePlaybackNativeInterface.BluePlaybackGetSerialNumber(pBluePlayback);
        }

        public string BlueVelvetVersion()
        {
            return BluePlaybackNativeInterface.BluePlaybackBlueVelvetVersion();
        }

        public int GetDevicecount()
        {
            return BluePlaybackNativeInterface.BluePlaybackGetDeviceCount(pBluePlayback);
        }

        public void Dispose()
        {}
    }

    internal class BluePlaybackNativeInterface
    {
        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern IntPtr BluePlaybackCreate();

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern Int32 BluePlaybackConfig(IntPtr pBluePlaybackObject, Int32 inDevNo,
                                                                                    Int32 inChannel,
                                                                                    Int32 inVidFmt,
                                                                                    Int32 inMemFmt,
                                                                                    Int32 inUpdFmt,
                                                                                    Int32 inVideoDestination,
                                                                                    Int32 inAudioDestination,
                                                                                    Int32 inAudioChannelMask);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern void BluePlaybackStart(IntPtr pBluePlaybackObject);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern void BluePlaybackStop(IntPtr pBluePlaybackObject);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern int BluePlaybackRender(IntPtr pBluePlaybackObject, IntPtr pBuffer);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern void BluePlaybackDestroy(IntPtr pBluePlaybackObject);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern int BluePlaybackSetCardProperty(IntPtr pBluePlaybackObject, int nProp, uint nValue);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern int BluePlaybackSetCardPropertyInt64(IntPtr pBluePlaybackObject, int nProp, Int64 nValue);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern int BluePlaybackQueryCardProperty(IntPtr pBluePlaybackObject, int nProp);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern ulong BluePlaybackGetMemorySize(IntPtr pBluePlaybackObject);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern string BluePlaybackGetSerialNumber(IntPtr pBluePlaybackObject);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern string BluePlaybackBlueVelvetVersion();

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern int BluePlaybackGetDeviceCount(IntPtr pBluePlaybackObject);
        
    }
}
