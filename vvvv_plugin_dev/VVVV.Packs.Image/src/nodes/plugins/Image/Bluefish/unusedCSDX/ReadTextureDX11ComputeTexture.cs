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
        /*Control FHiddenControl;
        SwapChain FSwapChain;
        RenderTargetView FRenderTargetView;*/

        //Texture2D FBackBuffer;
        Texture2D FSharedTexture;
        SlimDX.Direct3D11.Buffer FOutputBufferBack, FOutputBufferFront;
        SlimDX.Direct3D11.Buffer[] FStagingBufferBack;
        UnorderedAccessView FOutputView;
        Texture2D FTextureOut;
        Texture2D[] FTextureBack;
        DataBox FReadBackData;
        int FBackBufferWidth, FBackBufferHeight;
        int FCurrentBack;
        int FCurrentFront;

        ILogger FLogger;
        bool FDirectCopy = false;
        Stopwatch FLogTimer;
        double FAvgTime;
        double FMaxTime;
        uint FNumFrames;
        WorkerThread FThread = new WorkerThread();


		public ReadTextureDX11(uint handle, BlueFish_h.EMemoryFormat outFormat, ILogger FLogger)
        {
            if (handle == 0)
            {
                throw (new Exception("No shared texture handle set"));
            }
            this.FLogTimer = new Stopwatch();
            FLogTimer.Start();
            this.FLogger = FLogger;
			var texHandle = (IntPtr)unchecked((int)handle);

            this.FDevice = new SlimDX.Direct3D11.Device(DriverType.Hardware,DeviceCreationFlags.SingleThreaded);
            this.FSharedTexture = this.FDevice.OpenSharedResource<Texture2D>(texHandle);
            var profile = this.FDevice.FeatureLevel >= FeatureLevel.Level_11_0 ? "cs_5_0" : "cs_4_0";

            FLogger.Log(LogType.Message, this.FSharedTexture.Description.Width.ToString() + "x" + this.FSharedTexture.Description.Height.ToString());
            FLogger.Log(LogType.Message, "ArraySize " + this.FSharedTexture.Description.ArraySize.ToString());
            FLogger.Log(LogType.Message, "BindFlags " + this.FSharedTexture.Description.BindFlags.ToString());
            FLogger.Log(LogType.Message, "CpuAccessFlags " + this.FSharedTexture.Description.CpuAccessFlags.ToString());
            FLogger.Log(LogType.Message, "Format " + this.FSharedTexture.Description.Format.ToString());
            FLogger.Log(LogType.Message, "MipLevels " + this.FSharedTexture.Description.MipLevels.ToString());
            FLogger.Log(LogType.Message, "OptionFlags " + this.FSharedTexture.Description.OptionFlags.ToString());
            FLogger.Log(LogType.Message, "SampleDescription " + this.FSharedTexture.Description.SampleDescription.ToString());
            FLogger.Log(LogType.Message, "Usage " + this.FSharedTexture.Description.Usage.ToString());
            FLogger.Log(LogType.Message, "Profile " + profile);

            var pixelShaderSrc = "";
            var backBufferFormat = Format.R8G8B8A8_UNorm;
            var backTextureFormat = Format.R8G8B8A8_UNorm;
            var swapRG = false;
            this.FBackBufferWidth = this.FSharedTexture.Description.Width;
            this.FBackBufferHeight = this.FSharedTexture.Description.Height;
            switch (outFormat)
            {
                case BlueFish_h.EMemoryFormat.MEM_FMT_BGRA:
                    if (!IsBGRA(this.FSharedTexture.Description.Format) && !IsBGRX(this.FSharedTexture.Description.Format))
                        throw new Exception("Input texture doesn't have the correct format: BGRA8888");
                    FDirectCopy = true;
                    backBufferFormat = Format.B8G8R8A8_UNorm;
                    backTextureFormat = this.FSharedTexture.Description.Format;
                    break;
                case BlueFish_h.EMemoryFormat.MEM_FMT_RGBA:
                    if (!IsRGBA(this.FSharedTexture.Description.Format) && !IsBGRX(this.FSharedTexture.Description.Format))
                        throw new Exception("Input texture doesn't have the correct format: RGBA8888");
                    /*if (IsRGBA32(this.FSharedTexture.Description.Format))
                    {
                        this.FDirectCopy = true;
                    }
                    else
                    {*/
                        pixelShaderSrc = "PSPassthrough";
                    //}
                    backBufferFormat = Format.R8G8B8A8_UNorm;
                    backTextureFormat = backBufferFormat;
                    break;
                case BlueFish_h.EMemoryFormat.MEM_FMT_2VUY:
                    if (!IsRGBA(this.FSharedTexture.Description.Format) && !IsBGRA(this.FSharedTexture.Description.Format) && !IsBGRX(this.FSharedTexture.Description.Format))
                        throw new Exception("Input texture doesn't have the correct format: RGBA or BGRA");
                    pixelShaderSrc = "PSRGBA8888_to_2VUY";
                    this.FBackBufferWidth = this.FSharedTexture.Description.Width / 2;
                    backBufferFormat = Format.R8G8B8A8_UNorm;
                    backTextureFormat = backBufferFormat;
                    swapRG = IsBGRA(this.FSharedTexture.Description.Format);
                    break;
                case BlueFish_h.EMemoryFormat.MEM_FMT_YUVS:
                    if (!IsRGBA(this.FSharedTexture.Description.Format) && !IsBGRA(this.FSharedTexture.Description.Format) && !IsBGRX(this.FSharedTexture.Description.Format))
                        throw new Exception("Input texture doesn't have the correct format: RGBA");
                    pixelShaderSrc = "PSRGBA8888_to_YUVS";
                    this.FBackBufferWidth = this.FSharedTexture.Description.Width / 2;
                    backBufferFormat = Format.R8G8B8A8_UNorm;
                    backTextureFormat = backBufferFormat;
                    swapRG = IsBGRA(this.FSharedTexture.Description.Format);
                    break;
                case BlueFish_h.EMemoryFormat.MEM_FMT_VUYA_4444:
                    if (!IsRGBA(this.FSharedTexture.Description.Format) && !IsBGRA(this.FSharedTexture.Description.Format) && !IsBGRX(this.FSharedTexture.Description.Format))
                        throw new Exception("Input texture doesn't have the correct format: RGBA");
                    backBufferFormat = Format.R8G8B8A8_UNorm;
                    backTextureFormat = backBufferFormat;
                    pixelShaderSrc = "PSRGBA8888_to_VUYA_4444";
                    swapRG = IsBGRA(this.FSharedTexture.Description.Format);
                    break;
                case BlueFish_h.EMemoryFormat.MEM_FMT_RGB:
                    if (!IsRGBA(this.FSharedTexture.Description.Format) && !IsBGRA(this.FSharedTexture.Description.Format) && !IsBGRX(this.FSharedTexture.Description.Format))
                        throw new Exception("Input texture doesn't have the correct format: RGBA or BGRA");
                    if (this.FSharedTexture.Description.Width * 3 / 4 < ((float)this.FSharedTexture.Description.Width) * 3.0 / 4.0)
                        throw new Exception("Format size cannot be packed as RGB efficiently");
                    backBufferFormat = Format.R8G8B8A8_UNorm;
                    backTextureFormat = backBufferFormat;
                    this.FBackBufferWidth = this.FSharedTexture.Description.Width * 3 / 4;
                    if (!IsRGBA(this.FSharedTexture.Description.Format))
                    {
                        pixelShaderSrc = "PSRGBA8888_to_RGB888";
                    }
                    else
                    {
                        pixelShaderSrc = "PSRGBA8888_to_BGR888";
                    }
                    break;
                case BlueFish_h.EMemoryFormat.MEM_FMT_BGR:
                    if (!IsRGBA(this.FSharedTexture.Description.Format) && !IsBGRA(this.FSharedTexture.Description.Format) && !IsBGRX(this.FSharedTexture.Description.Format))
                        throw new Exception("Input texture doesn't have the correct format: RGBA or BGRA");
                    if (this.FSharedTexture.Description.Width * 3 / 4 < ((float)this.FSharedTexture.Description.Width) * 3.0 / 4.0)
                        throw new Exception("Format size cannot be packed as BGR efficiently");
                    backBufferFormat = Format.R8G8B8A8_UNorm;
                    backTextureFormat = backBufferFormat;
                    this.FBackBufferWidth = this.FSharedTexture.Description.Width * 3 / 4;
                    if (!IsRGBA(this.FSharedTexture.Description.Format))
                    {
                        pixelShaderSrc = "PSRGBA8888_to_BGR888";
                    }
                    else
                    {
                        pixelShaderSrc = "PSRGBA8888_to_RGB888";
                    }
                    break;
                case BlueFish_h.EMemoryFormat.MEM_FMT_V210:
                    if (!IsRGBA(this.FSharedTexture.Description.Format) && !IsBGRA(this.FSharedTexture.Description.Format) && !IsBGRX(this.FSharedTexture.Description.Format) && IsR10G10B10A2(this.FSharedTexture.Description.Format))
                        throw new Exception("Input texture doesn't have the correct format: RGBA");
                    if (this.FSharedTexture.Description.Width * 4 / 6 < ((float)this.FSharedTexture.Description.Width) * 4.0 / 6.0)
                        throw new Exception("Format size cannot be packed as V210 efficiently");
                    backBufferFormat = Format.R10G10B10A2_UNorm;
                    backTextureFormat = backBufferFormat;
                    if (this.FSharedTexture.Description.Format != Format.R10G10B10A2_UNorm)
                    {
                        swapRG = IsBGRA(this.FSharedTexture.Description.Format);
                        this.FBackBufferWidth = this.FSharedTexture.Description.Width * 4 / 6;
                        pixelShaderSrc = "PSRGBA8888_to_V210";
                    }
                    else
                    {
                        this.FDirectCopy = true;
                    }
                    break;
                case BlueFish_h.EMemoryFormat.MEM_FMT_Y210:
                    if (!IsRGBA(this.FSharedTexture.Description.Format) && !IsBGRA(this.FSharedTexture.Description.Format) && !IsBGRX(this.FSharedTexture.Description.Format) && IsR10G10B10A2(this.FSharedTexture.Description.Format))
                        throw new Exception("Input texture doesn't have the correct format: RGBA");
                    if (this.FSharedTexture.Description.Width * 4 / 6 < ((float)this.FSharedTexture.Description.Width) * 4.0 / 6.0)
                        throw new Exception("Format size cannot be packed as Y210 efficiently");
                    backBufferFormat = Format.R10G10B10A2_UNorm;
                    backTextureFormat = backBufferFormat;
                    if (this.FSharedTexture.Description.Format != Format.R10G10B10A2_UNorm)
                    {
                        swapRG = IsBGRA(this.FSharedTexture.Description.Format);
                        this.FBackBufferWidth = this.FSharedTexture.Description.Width * 4 / 6;
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

            /*this.FHiddenControl = new Control();
            this.FHiddenControl.Visible = false;
            this.FHiddenControl.Width = this.FBackBufferWidth;
            this.FHiddenControl.Height = this.FBackBufferHeight;
            var desc = new SwapChainDescription()
            {
                BufferCount = 4,
                ModeDescription = new ModeDescription(this.FBackBufferWidth, this.FBackBufferHeight, new Rational(120, 1), backBufferFormat),
                IsWindowed = false,
                OutputHandle = this.FHiddenControl.Handle,
                SampleDescription = new SampleDescription(1, 0),
                SwapEffect = SwapEffect.Discard,
                Usage = Usage.BackBuffer | Usage.RenderTargetOutput
            };

            SlimDX.Direct3D11.Device.CreateWithSwapChain(DriverType.Hardware, DeviceCreationFlags.SingleThreaded, desc, out this.FDevice, out this.FSwapChain);*/
            /*this.FBackBuffer = Texture2D.FromSwapChain<Texture2D>(this.FSwapChain, 0);
            this.FRenderTargetView = new RenderTargetView(this.FDevice, this.FBackBuffer);
            this.FDevice.ImmediateContext.OutputMerger.SetTargets(this.FRenderTargetView);
            this.FDevice.ImmediateContext.Rasterizer.SetViewports(new Viewport(0, 0, this.FBackBufferWidth, this.FBackBufferHeight, 0.0f, 1.0f));*/

            //this.FSharedTexture = this.FDevice.OpenSharedResource<Texture2D>(texHandle);

            
           /* var texDescription = this.FSharedTexture.Description;
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
            this.FTextureBack = new Texture2D[8];
            for (int i = 0; i < this.FTextureBack.Length; i++)
            {
                this.FTextureBack[i] = new Texture2D(this.FDevice, texDescription);
            }

            texDescription.Usage = ResourceUsage.Default;
            texDescription.BindFlags = BindFlags.RenderTarget;
            texDescription.CpuAccessFlags = CpuAccessFlags.None;*/

            FLogger.Log(LogType.Message, Environment.CurrentDirectory);

            if (!this.FDirectCopy)
            {
                //ShaderSignature inputSignature;
                //VertexShader vertexShader;
                //PixelShader pixelShader;
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
                /*using (var bytecode = ShaderBytecode.CompileFromFile("Shaders/ColorSpaceConversionDX11.hlsl", "VShader", "vs_4_0", ShaderFlags.None, EffectFlags.None))
                {
                    inputSignature = ShaderSignature.GetInputSignature(bytecode);
                    vertexShader = new VertexShader(this.FDevice, bytecode);
                }
                using (var bytecode = ShaderBytecode.CompileFromFile("Shaders/ColorSpaceConversionDX11.hlsl", pixelShaderSrc, "ps_4_0", ShaderFlags.None, EffectFlags.None, defines, null))
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
                var vertexBuffer = new SlimDX.Direct3D11.Buffer(this.FDevice, quad, 6 * 3 * sizeof(float) + 6 * 2 * sizeof(float), ResourceUsage.Default, BindFlags.VertexBuffer, CpuAccessFlags.None, ResourceOptionFlags.None, 0);*/

                var context = this.FDevice.ImmediateContext;
                //context.VertexShader.Set(vertexShader);
                //context.PixelShader.Set(pixelShader);
                /*context.InputAssembler.InputLayout = layout;
                context.InputAssembler.PrimitiveTopology = PrimitiveTopology.TriangleList;
                context.InputAssembler.SetVertexBuffers(0, new VertexBufferBinding(vertexBuffer, 5 * sizeof(float), 0));*/

                var samplerDesc = new SamplerDescription
                {
                    Filter = Filter.MinMagMipPoint,
                    AddressU = TextureAddressMode.Border,
                    AddressV = TextureAddressMode.Border,
                    AddressW = TextureAddressMode.Border,
                };

                var samplerState = SamplerState.FromDescription(this.FDevice, samplerDesc);

                //context.PixelShader.SetSampler(samplerState, 0);

                var shaderVars = new DataStream(4*sizeof(float),true,true);
                shaderVars.Write((float)this.FSharedTexture.Description.Width);
                shaderVars.Write((float)this.FSharedTexture.Description.Height);
                shaderVars.Write((float)this.FBackBufferWidth);
                shaderVars.Write((float)this.FBackBufferHeight);
                //shaderVars.Write((float)1.0 / (float)this.FSharedTexture.Description.Width);
                shaderVars.Position = 0;
                var varsBuffer = new SlimDX.Direct3D11.Buffer(this.FDevice,shaderVars,4*sizeof(float),ResourceUsage.Default, BindFlags.ConstantBuffer, CpuAccessFlags.None, ResourceOptionFlags.None, 0);
                //context.PixelShader.SetConstantBuffer(varsBuffer,0);



                ComputeShader computeShader;
                using (var bytecode = ShaderBytecode.CompileFromFile("C:\\DEV\\bluefish\\vvvv_plugin_dev\\VVVV.Packs.Image\\src\\nodes\\plugins\\Image\\Bluefish\\Shaders\\ColorSpaceConversionDX11Compute.hlsl", pixelShaderSrc, profile, ShaderFlags.None, EffectFlags.None, defines, null))
                {
                    computeShader = new ComputeShader(this.FDevice, bytecode);
                }

                context.ComputeShader.Set(computeShader);
                //context.ComputeShader.SetSampler(samplerState, 0);
                context.ComputeShader.SetConstantBuffer(varsBuffer, 0); 
                /*var shaderResourceView = new ShaderResourceView(this.FDevice, this.FSharedTexture);
                //context.ComputeShader.SetShaderResource(shaderResourceView, 0);

                var outBufferDesc = new BufferDescription
                {
                    Usage = ResourceUsage.Default,
                    SizeInBytes = this.FBackBufferWidth * this.FBackBufferHeight * sizeof(int),
                    BindFlags = BindFlags.UnorderedAccess | BindFlags.ShaderResource,                    
                    CpuAccessFlags = 0,
                    StructureByteStride = sizeof(int),
                    OptionFlags = ResourceOptionFlags.StructuredBuffer
                };
                this.FOutputBufferBack = new SlimDX.Direct3D11.Buffer(this.FDevice, outBufferDesc);
                this.FOutputBufferFront = new SlimDX.Direct3D11.Buffer(this.FDevice, outBufferDesc);
                var outViewDescription = new UnorderedAccessViewDescription
                {
                    ElementCount = this.FBackBufferWidth * this.FBackBufferHeight,
                    Format = Format.Unknown,
                    Dimension = UnorderedAccessViewDimension.Buffer
                };
                UnorderedAccessView outputView = new UnorderedAccessView(this.FDevice, this.FOutputBufferBack, outViewDescription);
                context.ComputeShader.SetUnorderedAccessView(outputView, 0);
                outputView.Dispose();*/


                var texDescription = this.FSharedTexture.Description;
                texDescription.MipLevels = 1;
                texDescription.ArraySize = 1;
                texDescription.SampleDescription = new SampleDescription(1, 0);
                texDescription.Usage = ResourceUsage.Default;
                texDescription.BindFlags = BindFlags.ShaderResource | BindFlags.UnorderedAccess;
                texDescription.CpuAccessFlags = 0;
                texDescription.OptionFlags = ResourceOptionFlags.None;
                texDescription.Width = this.FBackBufferWidth;
                texDescription.Height = this.FBackBufferHeight;
                texDescription.Format = backTextureFormat;
                this.FTextureOut = new Texture2D(this.FDevice, texDescription);
                FOutputView = new UnorderedAccessView(this.FDevice, this.FTextureOut);
                context.ComputeShader.SetUnorderedAccessView(FOutputView, 0);
                //context.ComputeShader.SetShaderResource(shaderResourceView, 0);

                texDescription.Usage = ResourceUsage.Staging;
                texDescription.BindFlags = BindFlags.None;
                texDescription.CpuAccessFlags = CpuAccessFlags.Read;
                this.FTextureBack = new Texture2D[8];
                for (int i = 0; i < this.FTextureBack.Length; i++)
                {
                    this.FTextureBack[i] = new Texture2D(this.FDevice, texDescription);
                }

                /*var shaderResourceView = new ShaderResourceView(this.FDevice, this.FSharedTexture);
                context.PixelShader.SetShaderResource(shaderResourceView, 0);
                shaderResourceView.Dispose();*/
            }


            /*BufferDescription stagingBufferDescription = new BufferDescription
            {
                BindFlags = BindFlags.None,
                CpuAccessFlags = CpuAccessFlags.Read,
                OptionFlags = ResourceOptionFlags.StructuredBuffer,
                SizeInBytes = this.FBackBufferWidth * this.FBackBufferHeight * sizeof(int),
                StructureByteStride = sizeof(int),
                Usage = ResourceUsage.Staging,
            };
            this.FStagingBufferBack = new SlimDX.Direct3D11.Buffer[2];
            for (int i = 0; i < this.FStagingBufferBack.Length; i++)
            {
                this.FStagingBufferBack[i] = new SlimDX.Direct3D11.Buffer(this.FDevice, stagingBufferDescription);
            }*/
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
                    context.UnmapSubresource(this.FTextureBack[this.FCurrentFront], 0);
                    this.FReadBackData = null;
                    this.FCurrentBack++;
                    this.FCurrentBack %= this.FTextureBack.Length;
                }
                //this.FCurrentFront = this.FCurrentBack;
                this.FCurrentFront = this.FCurrentBack + 6;
                this.FCurrentFront %= this.FTextureBack.Length;

                if (this.FDirectCopy)
                {
                    context.CopyResource(this.FSharedTexture, this.FTextureBack[this.FCurrentBack]);
                }
                else
                {
                    /*var outViewDescription = new UnorderedAccessViewDescription
                    {
                        ElementCount = this.FBackBufferWidth * this.FBackBufferHeight,
                        Format = Format.Unknown,
                        Dimension = UnorderedAccessViewDimension.Buffer
                    };
                    UnorderedAccessView outputView = new UnorderedAccessView(this.FDevice, this.FOutputBufferBack, outViewDescription);
                    context.ComputeShader.SetUnorderedAccessView(outputView, 0);
                    outputView.Dispose();*/
                    context.ComputeShader.SetUnorderedAccessView(FOutputView, 0);
                    context.Dispatch(this.FBackBufferWidth / 32, this.FBackBufferHeight / 30, 1);
                    context.ComputeShader.SetUnorderedAccessView(null, 0);
                    context.CopyResource(this.FTextureOut, this.FTextureBack[this.FCurrentBack]);
                    //FLogger.Log(LogType.Message, "Writting to " + this.FCurrentBack);
                    /*SlimDX.Direct3D11.Buffer aux = this.FOutputBufferBack;
                    this.FOutputBufferBack = this.FOutputBufferFront;
                    this.FOutputBufferFront = aux;*/
                }

                
                //Stopwatch Timer = new Stopwatch();
                //Timer.Start();
                IntPtr ret = (IntPtr)0;
                try
                {
                    //FLogger.Log(LogType.Message, "back: " + FCurrentBack + " front: " + FCurrentFront);
                    this.FReadBackData = context.MapSubresource(this.FTextureBack[this.FCurrentFront], 0, 0, MapMode.Read, SlimDX.Direct3D11.MapFlags.None);
                    //FLogger.Log(LogType.Message, "Reading from " + this.FCurrentFront);

                    ret = this.FReadBackData.Data.DataPointer;
                }
                catch (Exception e)
                {
                    FLogger.Log(e);
                }

                //Timer.Stop();
                //LogTime(Timer.Elapsed.TotalMilliseconds);
                return ret;
			}
			catch (Exception e)
            {
                FLogger.Log(LogType.Error, e.ToString());
				throw e;
			}
		}

        private void LogTime(double lastTime){
            /*if (lastTime > 10)
            {
                FLogger.Log(LogType.Error, "Error!!!!  Time: " + lastTime.ToString());
            }*/
            FAvgTime += lastTime;
            FNumFrames++;
            if (lastTime > FMaxTime)
            {
                FMaxTime = lastTime;
            }
            if (FLogTimer.ElapsedMilliseconds >= 10000)
            {
                FLogTimer.Reset();
                FAvgTime /= FNumFrames;
                FLogger.Log(LogType.Message, "Avg Time: " + FAvgTime);
                FLogger.Log(LogType.Message, "Max Time: " + FMaxTime);
                FAvgTime = 0;
                FNumFrames = 0;
                FMaxTime = 0;
                FLogTimer.Start();
            }
        }

		public void Dispose()
		{
            this.FSharedTexture.Dispose();
            //this.FBackBuffer.Dispose();
            //this.FRenderTargetView.Dispose();
            if (this.FReadBackData != null)
            {
                //this.FDevice.ImmediateContext.UnmapSubresource(this.FStagingBufferBack, 0);
                this.FReadBackData = null;
            }
            //this.FStagingBufferBack.Dispose();
            //this.FStagingBufferFront.Dispose();

            /*if (this.FReadBackData!=null)
            {
                this.FDevice.ImmediateContext.UnmapSubresource(this.FTextureBack[FCurrentFront], 0);
                this.FReadBackData = null;
            }
            foreach (var tex in this.FTextureBack)
            {
                tex.Dispose();
            }*/
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
