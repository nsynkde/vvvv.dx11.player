using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;

namespace VVVV.Nodes.DX11PlayerNode
{
    internal class NativeInterface
    {
        [DllImport("DX11PlayerNative.dll", SetLastError = false, CharSet = CharSet.Ansi)]
        internal static extern bool DX11Player_Available();

        [DllImport("DX11PlayerNative.dll", SetLastError = false, CharSet = CharSet.Ansi)]
        internal static extern IntPtr DX11Player_Create(string formatFile, int ringBufferSize);

        [DllImport("DX11PlayerNative.dll", SetLastError = false)]
        internal static extern void DX11Player_Destroy(IntPtr player);

        [DllImport("DX11PlayerNative.dll", SetLastError = false)]
        internal static extern void DX11Player_Update(IntPtr player);

        [DllImport("DX11PlayerNative.dll", SetLastError = false)]
        internal static extern IntPtr DX11Player_GetSharedHandle(IntPtr player, string nextFrame);

        [DllImport("DX11PlayerNative.dll", SetLastError = false)]
        internal static extern int DX11Player_GetUploadBufferSize(IntPtr player);

        [DllImport("DX11PlayerNative.dll", SetLastError = false)]
        internal static extern int DX11Player_GetWaitBufferSize(IntPtr player);

        [DllImport("DX11PlayerNative.dll", SetLastError = false)]
        internal static extern int DX11Player_GetRenderBufferSize(IntPtr player);

        [DllImport("DX11PlayerNative.dll", SetLastError = false)]
        internal static extern int DX11Player_GetPresentBufferSize(IntPtr player);

        [DllImport("DX11PlayerNative.dll", SetLastError = false)]
        internal static extern int DX11Player_GetDroppedFrames(IntPtr player);

        [DllImport("DX11PlayerNative.dll", SetLastError = false)]
        internal static extern int DX11Player_GetAvgLoadDurationMs(IntPtr player);

        [DllImport("DX11PlayerNative.dll", SetLastError = false)]
        internal static extern void DX11Player_SendNextFrameToLoad(IntPtr player, string nextFrame);

        [DllImport("DX11PlayerNative.dll", SetLastError = false)]
        [return: MarshalAs(UnmanagedType.I1)]
        internal static extern bool DX11Player_IsReady(IntPtr player);

        [DllImport("DX11PlayerNative.dll", SetLastError = false)]
        [return: MarshalAs(UnmanagedType.I1)]
        internal static extern bool DX11Player_GotFirstFrame(IntPtr player);

        [DllImport("DX11PlayerNative.dll", SetLastError = false)]
        internal static extern int DX11Player_GetStatus(IntPtr player);

        [DllImport("DX11PlayerNative.dll", SetLastError = false, CharSet = CharSet.Ansi,
            CallingConvention = CallingConvention.StdCall)]
        [return: MarshalAs(UnmanagedType.LPStr)]
        internal static extern string DX11Player_GetStatusMessage(IntPtr player);

        [DllImport("DX11PlayerNative.dll", SetLastError = false, CharSet = CharSet.Ansi,
            CallingConvention = CallingConvention.StdCall)]
        [return: MarshalAs(UnmanagedType.LPStr)]
        internal static extern string DX11Player_GetCurrentLoadFrame(IntPtr player);

        [DllImport("DX11PlayerNative.dll", SetLastError = false, CharSet = CharSet.Ansi,
            CallingConvention = CallingConvention.StdCall)]
        [return: MarshalAs(UnmanagedType.LPStr)]
        internal static extern string DX11Player_GetCurrentRenderFrame(IntPtr player);

        [DllImport("DX11PlayerNative.dll", CallingConvention = CallingConvention.StdCall)]
        internal static extern void DX11Player_SetSystemFrames(IntPtr player, String[] frames, int size);

        [DllImport("DX11PlayerNative.dll")]
        internal static extern void DX11Player_SetAlwaysShowLastFrame(IntPtr player, bool always);

        [DllImport("DX11PlayerNative.dll", SetLastError = false, CharSet = CharSet.Ansi)]
        internal static extern bool DX11Player_IsSameFormat(string formatFile1, string formatFile2);
    }
}
