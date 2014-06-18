using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading;
using System.Windows.Forms;
using SlimDX;
using SlimDX.Direct3D9;
using VVVV.PluginInterfaces.V2;
using VVVV.Core.Logging;
using System.Runtime.InteropServices;

namespace VVVV.Nodes.Bluefish
{
	public enum TextureType
	{
		None,
		RenderTarget,
		DepthStencil,
		Dynamic
	}

	/// <summary>
	/// A method for copying data back from a DX9Ex shared Surface on the GPU
	/// </summary>
	class ReadTexture : IDisposable
	{

		Direct3DEx FContext;
		DeviceEx FDevice;
		Control FHiddenControl;
		bool FInitialised = false;
        bool FBackLocked = false;
		int FInWidth;
        int FInHeight;
        int FOutWidth;
        int FOutHeight;
		IntPtr FHandle;
		Format FFormat;
		Usage FUsage;

        Texture FTextureShared;
        Texture[] FTexturePool;
        Texture[] FResizeTexturePool;
        int frontTexture, backTexture;
        //Surface FSurfaceOffscreen;

        ILogger FLogger;

        public bool FVSync = true;

		public ReadTexture(int inWidth, int inHeight, int outWidth, int outHeight, uint handle, EnumEntry formatEnum, EnumEntry usageEnum, ILogger FLogger)
		{
			Format format;
			if (formatEnum.Name == "INTZ")
				format = D3DX.MakeFourCC((byte)'I', (byte)'N', (byte)'T', (byte)'Z');
			else if (formatEnum.Name == "RAWZ")
				format = D3DX.MakeFourCC((byte)'R', (byte)'A', (byte)'W', (byte)'Z');
			else if (formatEnum.Name == "RESZ")
				format = D3DX.MakeFourCC((byte)'R', (byte)'E', (byte)'S', (byte)'Z');
			else if (formatEnum.Name == "No Specific")
				throw (new Exception("Texture mode not supported"));
			else
				format = (Format)Enum.Parse(typeof(Format), formatEnum, true);

			var usage = Usage.Dynamic;
            if (usageEnum.Index == (int)(TextureType.RenderTarget))
            {
                usage = Usage.RenderTarget;
                FLogger.Log(LogType.Message, "texture as renderTarget");
            }
            else if (usageEnum.Index == (int)(TextureType.DepthStencil))
            {
                usage = Usage.DepthStencil;
                FLogger.Log(LogType.Message, "texture as depthstencil");
            }
            else
            {
                FLogger.Log(LogType.Message, "texture as dynamic");
            }

			this.FInWidth = inWidth;
            this.FInHeight = inHeight;
            this.FOutWidth = outWidth;
            this.FOutHeight = outHeight;
			this.FHandle = (IntPtr)unchecked((int)handle);
			this.FFormat = format;
            this.FUsage = usage;
            FLogger.Log(LogType.Message, "shared tex shared handle " + this.FHandle.ToString());
            this.FLogger = FLogger;

			Initialise();
		}

        public ReadTexture(int inWidth, int inHeight, int outWidth, int outHeight, IntPtr handle, Format format, Usage usage, ILogger FLogger)
        {
            this.FInWidth = inWidth;
            this.FInHeight = inHeight;
            this.FOutWidth = outWidth;
            this.FOutHeight = outHeight;
			this.FHandle = handle;
			this.FFormat = format;
			this.FUsage = usage;

			Initialise();
		}

		void Initialise()
        {
            FLogger.Log(LogType.Message, "Initialize");
            if (this.FHandle == (IntPtr)0)
            {
                FLogger.Log(LogType.Message, "shared tex shared handle 0" + this.FHandle.ToString());
                throw (new Exception("No shared texture handle set"));
            }
			this.FContext = new Direct3DEx();

			this.FHiddenControl = new Control();
			this.FHiddenControl.Visible = false;
			this.FHiddenControl.Width = this.FInWidth;
			this.FHiddenControl.Height = this.FInHeight;
			
			var flags = CreateFlags.HardwareVertexProcessing | CreateFlags.Multithreaded | CreateFlags.PureDevice | CreateFlags.FpuPreserve;
			this.FDevice = new DeviceEx(FContext, 0, DeviceType.Hardware, this.FHiddenControl.Handle, flags, new PresentParameters()
			{
				BackBufferWidth = this.FInWidth,
				BackBufferHeight = this.FInHeight
			});

			this.FTextureShared = new Texture(this.FDevice, this.FInWidth, this.FInHeight, 1, this.FUsage, FFormat, Pool.Default, ref this.FHandle);
            this.FTexturePool = new Texture[8];
            for (int i = 0; i < this.FTexturePool.Length;i++ )
            {
                this.FTexturePool[i] = new Texture(this.FDevice, this.FOutWidth, this.FOutHeight, 1, Usage.Dynamic, FFormat, Pool.Default);
            } 
            if (FInWidth != FOutWidth || FInHeight != FOutHeight)
            {
                this.FResizeTexturePool = new Texture[8];
                for (int i = 0; i < this.FTexturePool.Length; i++)
                {
                    this.FResizeTexturePool[i] = new Texture(this.FDevice, this.FOutWidth, this.FOutHeight, 1, Usage.RenderTarget, FFormat, Pool.Default);
                }
            }
			/*this.FTextureBack = new Texture(this.FDevice, this.FWidth, this.FHeight, 1, Usage.Dynamic, FFormat, Pool.Default);
            this.FTextureFront = new Texture(this.FDevice, this.FWidth, this.FHeight, 1, Usage.Dynamic, FFormat, Pool.Default);*/


            var description = FTextureShared.GetLevelDescription(0);
			//this.FSurfaceOffscreen = Surface.CreateOffscreenPlainEx(FDevice, FWidth, FHeight, description.Format, Pool.SystemMemory, Usage.None);
			this.FInitialised = true;
		}

		/// <summary>
		/// Read back the data from the texture into a CPU buffer
		/// </summary>
		/// <param name="buffer"></param>
		public void ReadBack(Source Source)
		{
			try
            {
                //Stopwatch Timer = new Stopwatch();
                //Timer.Start();
                if (Source.DoneLastFrame())
                {
                    // Texture pool, send previous frame
                    /*if (FBackLocked)
                    {
                        this.FTexturePool[frontTexture].UnlockRectangle(0);
                        FBackLocked = false;
                    }

                    frontTexture = backTexture;
                    backTexture++;
                    backTexture %= this.FTexturePool.Length;

                    var rect = this.FTexturePool[frontTexture].LockRectangle(0, LockFlags.ReadOnly);
                    FBackLocked = true;
                    Source.SendFrame(rect.Data.DataPointer);

                    FDevice.StretchRectangle(this.FTextureShared.GetSurfaceLevel(0), this.FTexturePool[backTexture].GetSurfaceLevel(0), TextureFilter.None);*/

                    //Threaded texture upload, should be faster

                    if (FInWidth != FOutWidth || FInHeight != FOutHeight)
                    {
                        FDevice.StretchRectangle(this.FTextureShared.GetSurfaceLevel(0), this.FResizeTexturePool[backTexture].GetSurfaceLevel(0), TextureFilter.Linear);
                        FDevice.StretchRectangle(this.FResizeTexturePool[backTexture].GetSurfaceLevel(0), this.FTexturePool[backTexture].GetSurfaceLevel(0), TextureFilter.None);
                    }
                    else
                    {
                        FDevice.StretchRectangle(this.FTextureShared.GetSurfaceLevel(0), this.FTexturePool[backTexture].GetSurfaceLevel(0), TextureFilter.None);
                    }
                    Source.SendFrame(this.FTexturePool[backTexture]);
                    backTexture++;
                    backTexture %= this.FTexturePool.Length;

                    // 1 texture, send immediately, seems as fast as texture pool
                    /*if (FBackLocked)
                    {
                        this.FTexturePool[0].UnlockRectangle(0);
                        FBackLocked = false;
                    }

                    FDevice.StretchRectangle(this.FTextureShared.GetSurfaceLevel(0), this.FTexturePool[0].GetSurfaceLevel(0), TextureFilter.None);


                    var rect = this.FTexturePool[0].LockRectangle(0, LockFlags.ReadOnly);
                    FBackLocked = true;
                    Source.SendFrame(rect.Data.DataPointer);*/

                    //Source.SendFrame(this.FTexturePool[currentTexture]);
                    //FDevice.GetRenderTargetData(this.FTextureShared.GetSurfaceLevel(0), FSurfaceOffscreen);
                    //var rect = FSurfaceOffscreen.LockRectangle(LockFlags.ReadOnly);

                    /*var TextureAux = FTextureFront;
                    FTextureFront = FTextureBack;
                    FTextureBack = TextureAux;*/
                }
                //Timer.Stop();
                //FLogger.Log(LogType.Message, Timer.Elapsed.TotalMilliseconds.ToString());
                if (FVSync)
                {
                    Source.WaitSync();
                }
			}
			catch (Exception e)
            {
                FLogger.Log(LogType.Error, e.ToString());
				FDevice.EndScene();
				throw e;
			}
		}

        /// <summary>
        /// Read back the data from the texture into a CPU buffer
        /// </summary>
        /// <param name="buffer"></param>
        /*public void ReadBack(IntPtr data)
        {
            Stopwatch Timer = new Stopwatch();
            Timer.Start();
            try
            {
                FDevice.GetRenderTargetData(this.FTextureShared.GetSurfaceLevel(0), FSurfaceOffscreen);

                var rect = FSurfaceOffscreen.LockRectangle(LockFlags.ReadOnly);
                try
                {

                    //FLogger.Log(LogType.Message, "reading " + buffer.Length);
                    memcpy(data, rect.Data.DataPointer, this.FWidth * this.FHeight * 4);
                    
                    FSurfaceOffscreen.UnlockRectangle();
                }
                catch (Exception e)
                {
                    FLogger.Log(LogType.Error, e.ToString());
                    FSurfaceOffscreen.UnlockRectangle();
                    throw e;
                }
            }
            catch (Exception e)
            {
                FLogger.Log(LogType.Error, e.ToString());
                FDevice.EndScene();
                throw e;
            }
            Timer.Stop();
            Debug.Print(Timer.Elapsed.TotalMilliseconds.ToString());
        }*/

		public int BufferLength
		{
			get
			{
				return this.FOutWidth * this.FOutHeight * 4;
			}
		}

		public void Dispose()
		{
            FTextureShared.Dispose();
			//FSurfaceOffscreen.Dispose();
            for (int i = 0; i < FTexturePool.Length; i++)
            {
                FTexturePool[i].Dispose();
            }
			
			FContext.Dispose();
			FDevice.Dispose();
			FHiddenControl.Dispose();
		}

        [DllImport("msvcrt.dll", SetLastError = false)]
        static extern IntPtr memcpy(IntPtr dest, IntPtr src, int count);
	}
}
