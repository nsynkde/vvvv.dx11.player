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
        RenderTargetView[] FRenderTargetView;
        Texture2D[] FBackBuffer;
        Texture2D FSharedTexture;
        Texture2D[] FTextureBack;
        DataBox FReadBackData;
        int FCurrentBack;
        int FCurrentFront;
        int FRendererOutput;

        ILogger FLogger;
        bool FDirectCopy = false;



        public ReadTextureDX11(ulong handle, BlueFish_h.EMemoryFormat outFormat, ILogger FLogger)
        {
            if (handle == 0)
            {
                throw (new Exception("No shared texture handle set"));
            }
            this.FLogger = FLogger;

            this.FDevice = new SlimDX.Direct3D11.Device(DriverType.Hardware,DeviceCreationFlags.SingleThreaded);
            var auxSharedTexture = this.FDevice.OpenSharedResource<Texture2D>((IntPtr)handle);

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
            var swapRG = false;
            switch (outFormat)
            {
                case BlueFish_h.EMemoryFormat.MEM_FMT_BGRA:
                    if (!IsBGRA(auxSharedTexture.Description.Format) && !IsBGRX(auxSharedTexture.Description.Format))
                        throw new Exception("Input texture doesn't have the correct format: BGRA8888");
                    FDirectCopy = true;
                    backBufferFormat = Format.B8G8R8A8_UNorm;
                    backTextureFormat = auxSharedTexture.Description.Format;
                    break;
                case BlueFish_h.EMemoryFormat.MEM_FMT_RGBA:
                    if (!IsRGBA(auxSharedTexture.Description.Format) && !IsBGRX(auxSharedTexture.Description.Format))
                        throw new Exception("Input texture doesn't have the correct format: RGBA8888");
                    if (IsRGBA32(auxSharedTexture.Description.Format))
                    {
                        FDirectCopy = true;
                    }
                    else
                    {
                        pixelShaderSrc = "PSPassthrough";
                    }
                    backBufferFormat = Format.R8G8B8A8_UNorm;
                    backTextureFormat = backBufferFormat;
                    break;
                case BlueFish_h.EMemoryFormat.MEM_FMT_2VUY:
                    if (!IsRGBA(auxSharedTexture.Description.Format) && !IsBGRA(auxSharedTexture.Description.Format) && !IsBGRX(auxSharedTexture.Description.Format))
                        throw new Exception("Input texture doesn't have the correct format: RGBA or BGRA");
                    pixelShaderSrc = "PSRGBA8888_to_2VUY";
                    backBufferWidth = backBufferWidth / 2;
                    backBufferFormat = Format.R8G8B8A8_UNorm;
                    backTextureFormat = backBufferFormat;
                    swapRG = IsBGRA(auxSharedTexture.Description.Format);
                    break;
                case BlueFish_h.EMemoryFormat.MEM_FMT_YUVS:
                    if (!IsRGBA(auxSharedTexture.Description.Format) && !IsBGRA(auxSharedTexture.Description.Format) && !IsBGRX(auxSharedTexture.Description.Format))
                        throw new Exception("Input texture doesn't have the correct format: RGBA");
                    pixelShaderSrc = "PSRGBA8888_to_YUVS";
                    backBufferWidth = backBufferWidth / 2;
                    backBufferFormat = Format.R8G8B8A8_UNorm;
                    backTextureFormat = backBufferFormat;
                    swapRG = IsBGRA(auxSharedTexture.Description.Format);
                    break;
                case BlueFish_h.EMemoryFormat.MEM_FMT_VUYA_4444:
                    if (!IsRGBA(auxSharedTexture.Description.Format) && !IsBGRA(auxSharedTexture.Description.Format) && !IsBGRX(auxSharedTexture.Description.Format))
                        throw new Exception("Input texture doesn't have the correct format: RGBA");
                    backBufferFormat = Format.R8G8B8A8_UNorm;
                    backTextureFormat = backBufferFormat;
                    pixelShaderSrc = "PSRGBA8888_to_VUYA_4444";
                    swapRG = IsBGRA(auxSharedTexture.Description.Format);
                    break;
                case BlueFish_h.EMemoryFormat.MEM_FMT_RGB:
                    if (!IsRGBA(auxSharedTexture.Description.Format) && !IsBGRA(auxSharedTexture.Description.Format) && !IsBGRX(auxSharedTexture.Description.Format))
                        throw new Exception("Input texture doesn't have the correct format: RGBA or BGRA");
                    if (backBufferWidth * 3 / 4 < ((float)backBufferWidth) * 3.0 / 4.0)
                        throw new Exception("Format size cannot be packed as RGB efficiently");
                    backBufferFormat = Format.R8G8B8A8_UNorm;
                    backTextureFormat = backBufferFormat;
                    backBufferWidth = backBufferWidth * 3 / 4;
                    if (!IsRGBA(auxSharedTexture.Description.Format))
                    {
                        pixelShaderSrc = "PSRGBA8888_to_RGB888";
                    }
                    else
                    {
                        pixelShaderSrc = "PSRGBA8888_to_BGR888";
                    }
                    break;
                case BlueFish_h.EMemoryFormat.MEM_FMT_BGR:
                    if (!IsRGBA(auxSharedTexture.Description.Format) && !IsBGRA(auxSharedTexture.Description.Format) && !IsBGRX(auxSharedTexture.Description.Format))
                        throw new Exception("Input texture doesn't have the correct format: RGBA or BGRA");
                    if (backBufferWidth * 3 / 4 < ((float)backBufferWidth) * 3.0 / 4.0)
                        throw new Exception("Format size cannot be packed as BGR efficiently");
                    backBufferFormat = Format.R8G8B8A8_UNorm;
                    backTextureFormat = backBufferFormat;
                    backBufferWidth = backBufferWidth * 3 / 4;
                    if (!IsRGBA(auxSharedTexture.Description.Format))
                    {
                        pixelShaderSrc = "PSRGBA8888_to_BGR888";
                    }
                    else
                    {
                        pixelShaderSrc = "PSRGBA8888_to_RGB888";
                    }
                    break;
                case BlueFish_h.EMemoryFormat.MEM_FMT_V210:
                    if (!IsRGBA(auxSharedTexture.Description.Format) && !IsBGRA(auxSharedTexture.Description.Format) && !IsBGRX(auxSharedTexture.Description.Format) && IsR10G10B10A2(auxSharedTexture.Description.Format))
                        throw new Exception("Input texture doesn't have the correct format: RGBA");
                    if (backBufferWidth * 4 / 6 < ((float)backBufferWidth) * 4.0 / 6.0)
                        throw new Exception("Format size cannot be packed as V210 efficiently");
                    backBufferFormat = Format.R10G10B10A2_UNorm;
                    backTextureFormat = backBufferFormat;
                    if (auxSharedTexture.Description.Format != Format.R10G10B10A2_UNorm)
                    {
                        swapRG = IsBGRA(auxSharedTexture.Description.Format);
                        backBufferWidth = backBufferWidth * 4 / 6;
                        pixelShaderSrc = "PSRGBA8888_to_V210";
                    }
                    else
                    {
                        this.FDirectCopy = true;
                    }
                    break;
                case BlueFish_h.EMemoryFormat.MEM_FMT_Y210:
                    if (!IsRGBA(auxSharedTexture.Description.Format) && !IsBGRA(auxSharedTexture.Description.Format) && !IsBGRX(auxSharedTexture.Description.Format) && IsR10G10B10A2(auxSharedTexture.Description.Format))
                        throw new Exception("Input texture doesn't have the correct format: RGBA");
                    if (backBufferWidth * 4 / 6 < ((float)backBufferWidth) * 4.0 / 6.0)
                        throw new Exception("Format size cannot be packed as Y210 efficiently");
                    backBufferFormat = Format.R10G10B10A2_UNorm;
                    backTextureFormat = backBufferFormat;
                    if (auxSharedTexture.Description.Format != Format.R10G10B10A2_UNorm)
                    {
                        swapRG = IsBGRA(auxSharedTexture.Description.Format);
                        backBufferWidth = backBufferWidth * 4 / 6;
                        pixelShaderSrc = "PSRGBA8888_to_Y210";
                    }
                    else
                    {
                        this.FDirectCopy = true;
                    }
                    break;
                case BlueFish_h.EMemoryFormat.MEM_FMT_YUV_ALPHA:
                case BlueFish_h.EMemoryFormat.MEM_FMT_BV10:
                case BlueFish_h.EMemoryFormat.MEM_FMT_ARGB:
                case BlueFish_h.EMemoryFormat.MEM_FMT_BGRA_16_16_16_16:
                case BlueFish_h.EMemoryFormat.MEM_FMT_BGR_16_16_16:
                case BlueFish_h.EMemoryFormat.MEM_FMT_CINEON_LITTLE_ENDIAN:
                case BlueFish_h.EMemoryFormat.MEM_FMT_CINEON:
                case BlueFish_h.EMemoryFormat.MEM_FMT_V216:
                case BlueFish_h.EMemoryFormat.MEM_FMT_Y216:
                default:
                    throw new Exception("Unsupported output format " + outFormat.ToString());
            }

            this.FSharedTexture = auxSharedTexture;//.FDevice.OpenSharedResource<Texture2D>(texHandle);
            var texDescription = this.FSharedTexture.Description;
            texDescription.MipLevels = 1;
            texDescription.ArraySize = 1;
            texDescription.SampleDescription = new SampleDescription(1, 0);
            texDescription.Usage = ResourceUsage.Default;
            texDescription.BindFlags = BindFlags.RenderTarget;
            texDescription.CpuAccessFlags = CpuAccessFlags.None;
            texDescription.OptionFlags = ResourceOptionFlags.None;
            texDescription.Width = backBufferWidth;
            texDescription.Height = backBufferHeight;
            texDescription.Format = backTextureFormat;
            this.FBackBuffer = new Texture2D[4];
            this.FRenderTargetView = new RenderTargetView[4];
            for (int i = 0; i < this.FBackBuffer.Length; i++)
            {
                this.FBackBuffer[i] = new Texture2D(this.FDevice, texDescription);
                this.FRenderTargetView[i] = new RenderTargetView(this.FDevice, this.FBackBuffer[i]);
            }
            this.FDevice.ImmediateContext.OutputMerger.SetTargets(this.FRenderTargetView[0]);
            this.FDevice.ImmediateContext.Rasterizer.SetViewports(new Viewport(0, 0, backBufferWidth, backBufferHeight, 0.0f, 1.0f));


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
            this.FTextureBack = new Texture2D[4];
            for (int i = 0; i < this.FTextureBack.Length; i++)
            {
                this.FTextureBack[i] = new Texture2D(this.FDevice, texDescription);
            }

            FLogger.Log(LogType.Message, Environment.CurrentDirectory);

            if (!this.FDirectCopy)
            {
                ShaderSignature inputSignature;
                VertexShader vertexShader;
                PixelShader pixelShader;
                ShaderMacro[] defines = new ShaderMacro[1];
                defines[0] = new ShaderMacro();
                defines[0].Name = "SWAP_RB";
                if (swapRG)
                {
                    defines[0].Value = "1";
                }
                else
                {
                    defines[0].Value = "0";
                }
                using (var bytecode = ShaderBytecode.CompileFromFile("Shaders/ColorSpaceConversionDX11.hlsl", "VShader", "vs_5_0", ShaderFlags.None, EffectFlags.None))
                {
                    inputSignature = ShaderSignature.GetInputSignature(bytecode);
                    vertexShader = new VertexShader(this.FDevice, bytecode);
                }
                using (var bytecode = ShaderBytecode.CompileFromFile("Shaders/ColorSpaceConversionDX11.hlsl", pixelShaderSrc, "ps_5_0", ShaderFlags.None, EffectFlags.None, defines, null))
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

                var shaderVars = new DataStream(5*sizeof(float),true,true);
                shaderVars.Write((float)this.FSharedTexture.Description.Width);
                shaderVars.Write((float)this.FSharedTexture.Description.Height);
                shaderVars.Write((float)backBufferWidth);
                shaderVars.Write((float)backBufferHeight);
                shaderVars.Write((float)1.0 / (float)this.FSharedTexture.Description.Width);
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
                var context = this.FDevice.ImmediateContext;
                if (this.FReadBackData != null)
                {
                    context.UnmapSubresource(this.FTextureBack[FCurrentFront], 0);
                    this.FReadBackData = null;
                    FCurrentBack += 1;
                    FCurrentBack %= FTextureBack.Length;
                }
                FCurrentFront = FCurrentBack + 2;
                FCurrentFront %= FTextureBack.Length;



                if (this.FDirectCopy)
                {
                    context.CopyResource(this.FSharedTexture, this.FTextureBack[FCurrentBack]);
                }
                else
                {
                    var renderTargetView = this.FRenderTargetView[FRendererOutput];
                    this.FDevice.ImmediateContext.OutputMerger.SetTargets(renderTargetView);
                    context.Draw(6, 0);

                    var FrontBuffer = this.FBackBuffer[(FRendererOutput + 1) % this.FBackBuffer.Length];
                    context.CopyResource(FrontBuffer, this.FTextureBack[FCurrentBack]);
                    FRendererOutput++;
                    FRendererOutput %= this.FBackBuffer.Length;
                }


                var ret = (IntPtr)0;
                try
                {
                    this.FReadBackData = context.MapSubresource(this.FTextureBack[FCurrentFront], 0, 0, MapMode.Read, SlimDX.Direct3D11.MapFlags.None);
                    ret = this.FReadBackData.Data.DataPointer;
                }
                catch (Exception)
                {
                    ret = (IntPtr)0;
                }
                context.Flush();
                return ret;
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
            //this.FBackBuffer.Dispose();

            if (this.FReadBackData!=null)
            {
                this.FDevice.ImmediateContext.UnmapSubresource(this.FTextureBack[FCurrentFront], 0);
                this.FReadBackData = null;
            }
            foreach (var tex in this.FTextureBack)
            {
                tex.Dispose();
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

        private bool IsRGBA(Format format)
        {
            return format == Format.R16G16B16A16_Float ||
                format == Format.R16G16B16A16_SInt ||
                format == Format.R16G16B16A16_SNorm ||
                format == Format.R16G16B16A16_Typeless ||
                format == Format.R16G16B16A16_UInt ||
                format == Format.R16G16B16A16_UNorm ||
                format == Format.R32G32B32A32_Float ||
                format == Format.R32G32B32A32_SInt ||
                format == Format.R32G32B32A32_Typeless ||
                format == Format.R32G32B32A32_UInt ||
                format == Format.R8G8B8A8_SInt ||
                format == Format.R8G8B8A8_SNorm ||
                format == Format.R8G8B8A8_Typeless ||
                format == Format.R8G8B8A8_UInt ||
                format == Format.R8G8B8A8_UNorm ||
                format == Format.R8G8B8A8_UNorm_SRGB;
        }

        private bool IsRGBA32(Format format)
        {
            return format == Format.R8G8B8A8_SInt ||
                format == Format.R8G8B8A8_SNorm ||
                format == Format.R8G8B8A8_Typeless ||
                format == Format.R8G8B8A8_UInt ||
                format == Format.R8G8B8A8_UNorm ||
                format == Format.R8G8B8A8_UNorm_SRGB;

        }

        private bool IsBGRA(Format format)
        {
            return format == Format.B8G8R8A8_Typeless ||
                format == Format.B8G8R8A8_UNorm ||
                format == Format.B8G8R8A8_UNorm_SRGB;
        }

        private bool IsBGRX(Format format)
        {
            return format == Format.B8G8R8X8_Typeless ||
                format == Format.B8G8R8X8_UNorm ||
                format == Format.B8G8R8X8_UNorm_SRGB;
        }

        private bool IsR10G10B10A2(Format format)
        {
            return format == Format.R10G10B10_XR_Bias_A2_UNorm ||
                format == Format.R10G10B10A2_Typeless ||
                format == Format.R10G10B10A2_UInt ||
                format == Format.R10G10B10A2_UNorm;
        }
	}
}
