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
            BluePlaybackNativeInterface.BluePlaybackStop(pBluePlayback);
        }

        public int BluePlaybackInterfaceWaitSync()
        {
            return BluePlaybackNativeInterface.BluePlaybackWaitSync(pBluePlayback);
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

        public int BluePlaybackQueryULongCardProperty(int nProp, out ulong outValue)
        {
            return BluePlaybackNativeInterface.BluePlaybackQueryULongCardProperty(pBluePlayback, nProp, out outValue);
        }

        public int BluePlaybackQueryUIntCardProperty(int nProp, out uint outValue)
        {
            return BluePlaybackNativeInterface.BluePlaybackQueryUIntCardProperty(pBluePlayback, nProp, out outValue);
        }

        public ulong BluePlaybackGetMemorySize()
        {
            return BluePlaybackNativeInterface.BluePlaybackGetMemorySize(pBluePlayback);
        }

        public string BluePlaybackGetSerialNumber()
        {
            
            string str = Marshal.PtrToStringAnsi(BluePlaybackNativeInterface.BluePlaybackGetSerialNumber(pBluePlayback));
            return str;
        }

        public string BlueVelvetVersion()
        {
            return BluePlaybackNativeInterface.BluePlaybackBlueVelvetVersion();
        }

        public int GetDevicecount()
        {
            return BluePlaybackNativeInterface.BluePlaybackGetDeviceCount(pBluePlayback);
        }

        public ulong BluePlaybackGetPixelsPerLine()
        {
            return BluePlaybackNativeInterface.BluePlaybackGetPixelsPerLine(pBluePlayback);
        }

        public ulong BluePlaybackGetVideoLines()
        {
            return BluePlaybackNativeInterface.BluePlaybackGetVideoLines(pBluePlayback);
        }

        public ulong BluePlaybackGetBytesPerLine()
        {
            return BluePlaybackNativeInterface.BluePlaybackGetBytesPerLine(pBluePlayback);
        }

        public ulong BluePlaybackGetBytesPerFrame()
        {
            return BluePlaybackNativeInterface.BluePlaybackGetBytesPerFrame(pBluePlayback);
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
        internal static extern int BluePlaybackWaitSync(IntPtr pBluePlaybackObject);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern int BluePlaybackRender(IntPtr pBluePlaybackObject, IntPtr pBuffer);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern void BluePlaybackDestroy(IntPtr pBluePlaybackObject);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern int BluePlaybackSetCardProperty(IntPtr pBluePlaybackObject, int nProp, uint nValue);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern int BluePlaybackSetCardPropertyInt64(IntPtr pBluePlaybackObject, int nProp, Int64 nValue);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern int BluePlaybackQueryULongCardProperty(IntPtr pBluePlaybackObject, int nProp, out ulong outPropValue);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern int BluePlaybackQueryUIntCardProperty(IntPtr pBluePlaybackObject, int nProp, out uint outPropValue);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern ulong BluePlaybackGetMemorySize(IntPtr pBluePlaybackObject);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern IntPtr BluePlaybackGetSerialNumber(IntPtr pBluePlaybackObject);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern string BluePlaybackBlueVelvetVersion();

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern int BluePlaybackGetDeviceCount(IntPtr pBluePlaybackObject);
        
        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern ulong BluePlaybackGetPixelsPerLine(IntPtr pBPHandle);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern ulong BluePlaybackGetVideoLines(IntPtr pBPHandle);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern ulong BluePlaybackGetBytesPerLine(IntPtr pBPHandle);

        [DllImport("BluePlayback.dll", SetLastError = false)]
        internal static extern ulong BluePlaybackGetBytesPerFrame(IntPtr pBPHandle);
    }
}
