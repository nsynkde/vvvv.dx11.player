using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading;
using System.Windows.Forms;
using SlimDX;
using SlimDX.Direct3D11;
using SlimDX.DXGI;
using VVVV.PluginInterfaces.V2;
using VVVV.Core.Logging;
using System.Runtime.InteropServices;
using SlimDX.D3DCompiler;

namespace VVVV.Nodes.Bluefish
{

	/// <summary>
	/// A method for copying data back from a DX9Ex shared Surface on the GPU
	/// </summary>
	class ReadTextureDX11 : IDisposable
	{
        SlimDX.Direct3D11.Device FDevice;
        Control FHiddenControl;
        SwapChain FSwapChain;
        RenderTargetView FRenderTargetView;

        Texture2D FBackBuffer;
        Texture2D FSharedTexture;
        Texture2D[] FTexturePool;
        DataBox FReadBackData;

        ILogger FLogger;
        bool FDirectCopy = false;



		public ReadTextureDX11(uint handle, BlueFish_h.EMemoryFormat outFormat, ILogger FLogger)
        {
            if (handle == 0)
            {
                throw (new Exception("No shared texture handle set"));
            }
            this.FLogger = FLogger;
			var texHandle = (IntPtr)unchecked((int)handle);


            var auxDevice = new SlimDX.Direct3D11.Device(DriverType.Hardware);
            var auxSharedTexture = auxDevice.OpenSharedResource<Texture2D>(texHandle);

            FLogger.Log(LogType.Message, auxSharedTexture.Description.Width.ToString() + "x" + auxSharedTexture.Description.Height.ToString());
            FLogger.Log(LogType.Message, "ArraySize " + auxSharedTexture.Description.ArraySize.ToString());
            FLogger.Log(LogType.Message, "BindFlags " + auxSharedTexture.Description.BindFlags.ToString());
            FLogger.Log(LogType.Message, "CpuAccessFlags " + auxSharedTexture.Description.CpuAccessFlags.ToString());
            FLogger.Log(LogType.Message, "Format " + auxSharedTexture.Description.Format.ToString());
            FLogger.Log(LogType.Message, "MipLevels " + auxSharedTexture.Description.MipLevels.ToString());
            FLogger.Log(LogType.Message, "OptionFlags " + auxSharedTexture.Description.OptionFlags.ToString());
            FLogger.Log(LogType.Message, "SampleDescription " + auxSharedTexture.Description.SampleDescription.ToString());
            FLogger.Log(LogType.Message, "Usage " + auxSharedTexture.Description.Usage.ToString());

            var pixelShaderSrc = "";
            var backBufferFormat = Format.R8G8B8A8_UNorm;
            var backTextureFormat = Format.R8G8B8A8_UNorm;
            var backBufferWidth = auxSharedTexture.Description.Width;
            var backBufferHeight = auxSharedTexture.Description.Height;
            switch (outFormat)
            {
                case BlueFish_h.EMemoryFormat.MEM_FMT_BGRA:
                    if (auxSharedTexture.Description.Format != Format.B8G8R8A8_UNorm && auxSharedTexture.Description.Format != Format.B8G8R8X8_UNorm)
                        throw new Exception("Input texture doesn't have the correct format " + Format.B8G8R8A8_UNorm.ToString());
                    FDirectCopy = true;
                    backBufferFormat = Format.B8G8R8A8_UNorm;
                    backTextureFormat = auxSharedTexture.Description.Format;
                    break;
                case BlueFish_h.EMemoryFormat.MEM_FMT_RGBA:
                    if (auxSharedTexture.Description.Format != Format.R8G8B8A8_UNorm)
                        throw new Exception("Input texture doesn't have the correct format " + Format.R8G8B8A8_UNorm.ToString());
                    FDirectCopy = true;
                    backBufferFormat = Format.R8G8B8A8_UNorm;
                    backTextureFormat = backBufferFormat;
                    break;
                case BlueFish_h.EMemoryFormat.MEM_FMT_BGRA_16_16_16_16:
                    if (auxSharedTexture.Description.Format != Format.R16G16B16A16_Float)
                        throw new Exception("Input texture doesn't have the correct format " + Format.R16G16B16A16_Float.ToString());
                    pixelShaderSrc = "SwapRB";
                    backBufferFormat = Format.R16G16B16A16_UInt;
                    backTextureFormat = Format.R16G16B16A16_Float;
                    break;
                case BlueFish_h.EMemoryFormat.MEM_FMT_BGR_16_16_16:
                    if (auxSharedTexture.Description.Format != Format.R16G16B16A16_Float)
                        throw new Exception("Input texture doesn't have the correct format " + Format.R16G16B16A16_Float.ToString());
                    if (backBufferWidth * 3 / 4 < ((float)backBufferWidth) * 3.0 / 4.0)
                        throw new Exception("Format size cannot be packed as RGB efficiently");
                    backBufferFormat = Format.R16G16B16A16_Float;
                    backTextureFormat = backBufferFormat;
                    backBufferWidth = backBufferWidth * 3 / 4;
                    pixelShaderSrc = "PSRGBA8888_to_BGR888";
                    break;
                case BlueFish_h.EMemoryFormat.MEM_FMT_2VUY:
                    if (auxSharedTexture.Description.Format != Format.R8G8B8A8_UNorm)
                        throw new Exception("Input texture doesn't have the correct format " + Format.R8G8B8A8_UNorm.ToString());
                    pixelShaderSrc = "PSRGBA8888_to_2VUY";
                    backBufferWidth = backBufferWidth / 2;
                    backBufferFormat = Format.R8G8B8A8_UNorm;
                    backTextureFormat = backBufferFormat;
                    break;
                case BlueFish_h.EMemoryFormat.MEM_FMT_YUVS:
                    if (auxSharedTexture.Description.Format != Format.R8G8B8A8_UNorm)
                        throw new Exception("Input texture doesn't have the correct format " + Format.R8G8B8A8_UNorm.ToString());
                    pixelShaderSrc = "PSRGBA8888_to_YUVS";
                    backBufferWidth = backBufferWidth / 2;
                    backBufferFormat = Format.R8G8B8A8_UNorm;
                    backTextureFormat = backBufferFormat;
                    break;
                case BlueFish_h.EMemoryFormat.MEM_FMT_VUYA_4444:
                    if (auxSharedTexture.Description.Format != Format.R8G8B8A8_UNorm)
                        throw new Exception("Input texture doesn't have the correct format " + Format.R8G8B8A8_UNorm.ToString());
                    backBufferFormat = Format.R8G8B8A8_UNorm;
                    backTextureFormat = backBufferFormat;
                    pixelShaderSrc = "PSRGBA8888_to_VUYA_4444";
                    break;
                case BlueFish_h.EMemoryFormat.MEM_FMT_YUV_ALPHA:
                    if (auxSharedTexture.Description.Format != Format.R8G8B8A8_UNorm)
                        throw new Exception("Input texture doesn't have the correct format " + Format.R8G8B8A8_UNorm.ToString());
                    backBufferFormat = Format.R8G8B8A8_UNorm;
                    backTextureFormat = backBufferFormat;
                    pixelShaderSrc = "PSRGBA8888_to_YUV_ALPHA";
                    break;
                case BlueFish_h.EMemoryFormat.MEM_FMT_RGB:
                    if (auxSharedTexture.Description.Format != Format.R8G8B8A8_UNorm &&
                        auxSharedTexture.Description.Format != Format.B8G8R8A8_UNorm &&
                        auxSharedTexture.Description.Format != Format.B8G8R8X8_UNorm)
                        throw new Exception("Input texture doesn't have the correct format " + Format.R8G8B8A8_UNorm.ToString());
                    if (backBufferWidth * 3 / 4 < ((float)backBufferWidth) * 3.0 / 4.0)
                        throw new Exception("Format size cannot be packed as RGB efficiently");
                    backBufferFormat = Format.R8G8B8A8_UNorm;
                    backTextureFormat = backBufferFormat;
                    backBufferWidth = backBufferWidth * 3 / 4;
                    if (auxSharedTexture.Description.Format == Format.R8G8B8A8_UNorm)
                    {
                        pixelShaderSrc = "PSRGBA8888_to_RGB888";
                    }
                    else
                    {
                        pixelShaderSrc = "PSRGBA8888_to_BGR888";
                    }
                    break;
                case BlueFish_h.EMemoryFormat.MEM_FMT_BGR:
                    if (auxSharedTexture.Description.Format != Format.R8G8B8A8_UNorm &&
                        auxSharedTexture.Description.Format != Format.B8G8R8A8_UNorm &&
                        auxSharedTexture.Description.Format != Format.B8G8R8X8_UNorm)
                        throw new Exception("Input texture doesn't have the correct format " + Format.R8G8B8A8_UNorm.ToString());
                    if (backBufferWidth * 3 / 4 < ((float)backBufferWidth) * 3.0 / 4.0)
                        throw new Exception("Format size cannot be packed as RGB efficiently");
                    backBufferFormat = Format.R8G8B8A8_UNorm;
                    backTextureFormat = backBufferFormat;
                    backBufferWidth = backBufferWidth * 3 / 4;
                    if (auxSharedTexture.Description.Format == Format.R8G8B8A8_UNorm)
                    {
                        pixelShaderSrc = "PSRGBA8888_to_BGR888";
                    }
                    else
                    {
                        pixelShaderSrc = "PSRGBA8888_to_RGB888";
                    }
                    break;
                case BlueFish_h.EMemoryFormat.MEM_FMT_V210:
                    if (auxSharedTexture.Description.Format != Format.R8G8B8A8_UNorm)
                        throw new Exception("Input texture doesn't have the correct format " + Format.R8G8B8A8_UNorm.ToString());
                    if (backBufferWidth * 4 / 6 < ((float)backBufferWidth) * 4.0 / 6.0)
                        throw new Exception("Format size cannot be packed as V210 efficiently");
                    backBufferFormat = Format.R10G10B10A2_UNorm;
                    backTextureFormat = backBufferFormat;
                    backBufferWidth = backBufferWidth * 4 / 6;
                    pixelShaderSrc = "PSRGBA8888_to_V210";
                    break;
                case BlueFish_h.EMemoryFormat.MEM_FMT_Y210:
                    if (auxSharedTexture.Description.Format != Format.R8G8B8A8_UNorm)
                        throw new Exception("Input texture doesn't have the correct format " + Format.R8G8B8A8_UNorm.ToString());
                    if (backBufferWidth * 4 / 6 < ((float)backBufferWidth) * 4.0 / 6.0)
                        throw new Exception("Format size cannot be packed as V210 efficiently");
                    backBufferFormat = Format.R10G10B10A2_UNorm;
                    backTextureFormat = backBufferFormat;
                    backBufferWidth = backBufferWidth * 4 / 6;
                    pixelShaderSrc = "PSRGBA8888_to_Y210";
                    break;
                case BlueFish_h.EMemoryFormat.MEM_FMT_ARGB:
                case BlueFish_h.EMemoryFormat.MEM_FMT_BV10:
                case BlueFish_h.EMemoryFormat.MEM_FMT_CINEON_LITTLE_ENDIAN:
                case BlueFish_h.EMemoryFormat.MEM_FMT_CINEON:
                case BlueFish_h.EMemoryFormat.MEM_FMT_V216:
                case BlueFish_h.EMemoryFormat.MEM_FMT_Y216:
                default:
                    throw new Exception("Unsupported output format " + outFormat.ToString());
            }
            auxSharedTexture.Dispose();
            auxDevice.Dispose();

            this.FHiddenControl = new Control();
            this.FHiddenControl.Visible = false;
            this.FHiddenControl.Width = backBufferWidth;
            this.FHiddenControl.Height = backBufferHeight;
            var desc = new SwapChainDescription()
            {
                BufferCount = 1,
                ModeDescription = new ModeDescription(backBufferWidth, backBufferHeight, new Rational(60, 1), backBufferFormat),
                IsWindowed = true,
                OutputHandle = this.FHiddenControl.Handle,
                SampleDescription = new SampleDescription(1, 0),
                SwapEffect = SwapEffect.Discard,
                Usage = Usage.RenderTargetOutput
            };

            SlimDX.Direct3D11.Device.CreateWithSwapChain(DriverType.Hardware, DeviceCreationFlags.Debug, desc, out this.FDevice, out this.FSwapChain);
            this.FBackBuffer = Texture2D.FromSwapChain<Texture2D>(this.FSwapChain, 0);
            this.FRenderTargetView = new RenderTargetView(this.FDevice, this.FBackBuffer);
            this.FDevice.ImmediateContext.OutputMerger.SetTargets(this.FRenderTargetView);
            this.FDevice.ImmediateContext.Rasterizer.SetViewports(new Viewport(0, 0, backBufferWidth, backBufferHeight, 0.0f, 1.0f));

            this.FSharedTexture = this.FDevice.OpenSharedResource<Texture2D>(texHandle);

            
            this.FTexturePool = new Texture2D[8];
            var texDescription = this.FSharedTexture.Description;
            texDescription.MipLevels = 1;
            texDescription.ArraySize = 1;
            texDescription.SampleDescription = new SampleDescription(1, 0);
            texDescription.Usage = ResourceUsage.Staging;
            texDescription.BindFlags = BindFlags.None;
            texDescription.CpuAccessFlags = CpuAccessFlags.Read;
            texDescription.OptionFlags = ResourceOptionFlags.None;
            texDescription.Width = backBufferWidth;
            texDescription.Height = backBufferHeight;
            texDescription.Format = backTextureFormat;
            for (int i = 0; i < this.FTexturePool.Length;i++ )
            {
                this.FTexturePool[i] = new Texture2D(this.FDevice, texDescription);
            }

            texDescription.Usage = ResourceUsage.Default;
            texDescription.BindFlags = BindFlags.RenderTarget;
            texDescription.CpuAccessFlags = CpuAccessFlags.None;

            //FLogger.Log(LogType.Message, Environment.CurrentDirectory);

            if (!this.FDirectCopy)
            {
                ShaderSignature inputSignature;
                VertexShader vertexShader;
                PixelShader pixelShader;
                using (var bytecode = ShaderBytecode.CompileFromFile("Shaders/ColorSpaceConversionDX11.hlsl", "VShader", "vs_4_0", ShaderFlags.None, EffectFlags.None))
                {
                    inputSignature = ShaderSignature.GetInputSignature(bytecode);
                    vertexShader = new VertexShader(this.FDevice, bytecode);
                }
                using (var bytecode = ShaderBytecode.CompileFromFile("Shaders/ColorSpaceConversionDX11.hlsl", pixelShaderSrc, "ps_4_0", ShaderFlags.None, EffectFlags.None))
                {
                    pixelShader = new PixelShader(this.FDevice, bytecode);
                }

                var quad = new DataStream(6 * 3 * sizeof(float) + 6 * 2 * sizeof(float), true, true);
                quad.Write(new Vector3(-1.0f, 1.0f, 0.5f));
                quad.Write(new Vector2(0.0f, 0.0f));
                quad.Write(new Vector3(1.0f, 1.0f, 0.5f));
                quad.Write(new Vector2(1.0f, 0.0f));
                quad.Write(new Vector3(-1.0f, -1.0f, 0.5f));
                quad.Write(new Vector2(0.0f, 1.0f));


                quad.Write(new Vector3(-1.0f, -1.0f, 0.5f));
                quad.Write(new Vector2(0.0f, 1.0f));
                quad.Write(new Vector3(1.0f, 1.0f, 0.5f));
                quad.Write(new Vector2(1.0f, 0.0f));
                quad.Write(new Vector3(1.0f, -1.0f, 0.5f));
                quad.Write(new Vector2(1.0f, 1.0f));
                quad.Position = 0;

                var elements = new[] { new InputElement("POSITION", 0, Format.R32G32B32_Float, 0), new InputElement("TEXCOORD", 0, Format.R32G32_Float, 3 * sizeof(float), 0) };
                var layout = new InputLayout(this.FDevice, inputSignature, elements);
                var vertexBuffer = new SlimDX.Direct3D11.Buffer(this.FDevice, quad, 6 * 3 * sizeof(float) + 6 * 2 * sizeof(float), ResourceUsage.Default, BindFlags.VertexBuffer, CpuAccessFlags.None, ResourceOptionFlags.None, 0);

                var context = this.FDevice.ImmediateContext;
                context.VertexShader.Set(vertexShader);
                context.PixelShader.Set(pixelShader);
                context.InputAssembler.InputLayout = layout;
                context.InputAssembler.PrimitiveTopology = PrimitiveTopology.TriangleList;
                context.InputAssembler.SetVertexBuffers(0, new VertexBufferBinding(vertexBuffer, 5 * sizeof(float), 0));

                var samplerDesc = new SamplerDescription
                {
                    Filter = Filter.MinMagPointMipLinear,
                    AddressU = TextureAddressMode.Wrap,
                    AddressV = TextureAddressMode.Wrap,
                    AddressW = TextureAddressMode.Wrap,
                };

                var samplerState = SamplerState.FromDescription(this.FDevice, samplerDesc);

                context.PixelShader.SetSampler(samplerState, 0);

                var shaderVars = new DataStream(4*sizeof(float),true,true);
                shaderVars.Write((float)this.FSharedTexture.Description.Width);
                shaderVars.Write((float)this.FSharedTexture.Description.Height);
                shaderVars.Write((float)backBufferWidth);
                shaderVars.Write((float)backBufferHeight);
                shaderVars.Position = 0;
                var varsBuffer = new SlimDX.Direct3D11.Buffer(this.FDevice,shaderVars,4*sizeof(float),ResourceUsage.Default, BindFlags.ConstantBuffer, CpuAccessFlags.None, ResourceOptionFlags.None, 0);
                context.PixelShader.SetConstantBuffer(varsBuffer,0);


                var shaderResourceView = new ShaderResourceView(this.FDevice, this.FSharedTexture);
                context.PixelShader.SetShaderResource(shaderResourceView, 0);
                shaderResourceView.Dispose();
            }
		}

		/// <summary>
		/// Read back the data from the texture into a CPU buffer
		/// </summary>
		/// <param name="buffer"></param>
		public IntPtr ReadBack()
		{
			try
            {
                //Stopwatch Timer = new Stopwatch();
                //Timer.Start();

                var context = this.FDevice.ImmediateContext;
                if (this.FReadBackData != null)
                {
                    context.UnmapSubresource(this.FTexturePool[0], 0);
                    this.FReadBackData = null;
                }
                if (this.FDirectCopy)
                {
                    context.CopyResource(this.FSharedTexture, this.FTexturePool[0]);
                }
                else
                {
                    context.Draw(6, 0);
                    FSwapChain.Present(0, PresentFlags.None);
                    context.CopyResource(this.FBackBuffer, this.FTexturePool[0]);
                }
                this.FReadBackData = context.MapSubresource(this.FTexturePool[0], 0, 0, MapMode.Read, SlimDX.Direct3D11.MapFlags.None);
                return this.FReadBackData.Data.DataPointer;
			}
			catch (Exception e)
            {
                FLogger.Log(LogType.Error, e.ToString());
				throw e;
			}
		}

		public void Dispose()
		{
            this.FSharedTexture.Dispose();
            this.FBackBuffer.Dispose();
            this.FRenderTargetView.Dispose();

            if (this.FReadBackData!=null)
            {
                this.FDevice.ImmediateContext.UnmapSubresource(this.FTexturePool[0], 0);
                this.FReadBackData = null;
            }
            for (int i = 0; i < FTexturePool.Length; i++)
            {
                FTexturePool[i].Dispose();
            }
            this.FDevice.Dispose();
        }

        public Format TexFormat
        {
            get
            {
                return this.FSharedTexture.Description.Format;
            }
        }

        public int InputTexWidth
        {
            get
            {
                return FSharedTexture.Description.Width;
            }
        }

        public int InputTexHeight
        {
            get
            {
                return FSharedTexture.Description.Height;
            }
        }
	}
}
