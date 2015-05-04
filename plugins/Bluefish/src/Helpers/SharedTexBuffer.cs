using SlimDX.Direct3D11;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace VVVV.Nodes.Bluefish.src.Helpers
{
    class SharedTexBuffer
    {
        private Texture2D[] FRingBuffer;
        private IntPtr[] FSharedHandles;
        private Texture2D FInTexture;
        private int FCurrentFront, FCurrentBack;
        private Device FDevice;

        public SharedTexBuffer(Texture2D inTexture, Device device, int ringBufferSize)
        {
            FInTexture = inTexture;
            FDevice = device;
            Texture2DDescription desc = inTexture.Description;
            desc.BindFlags = BindFlags.ShaderResource;
            desc.OptionFlags = ResourceOptionFlags.Shared;
            desc.MipLevels = 1;

            FRingBuffer = new Texture2D[ringBufferSize];
            FSharedHandles = new IntPtr[ringBufferSize];
            for (int i = 0; i < FRingBuffer.Length; i++)
            {
                FRingBuffer[i] = new Texture2D(device, desc);
                var SharedResource = new SlimDX.DXGI.Resource(FRingBuffer[i]);
                FSharedHandles[i] = SharedResource.SharedHandle;

            }


            FCurrentBack = 0;
            FCurrentFront = FRingBuffer.Length / 2 - 1;
            if (FCurrentFront < 0) FCurrentFront = 0;
        }

        public void OnRender()
        {
            FDevice.ImmediateContext.CopyResource(this.FInTexture, FRingBuffer[FCurrentBack]);
            FCurrentBack+=1;
            FCurrentBack %= FRingBuffer.Length;
            FCurrentFront += 1;
            FCurrentFront %= FRingBuffer.Length;
        }

        public IntPtr FrontSharedHandle
        {
            get
            {
                return FSharedHandles[FCurrentFront];
            }
        }

    }
}
