//@author: vux
//@help: template for texture fx
//@tags: texture
//@credits: 

Texture2D tex0;// : PREVIOUS;
SamplerState s0;

float2 R:TARGETSIZE;
/*float InputWidth;
float InputHeight <string uiname="Input Height";> = 1080.0;
float OutputWidth <string uiname="Output Width";> = 1920.0;
float OutputHeight <string uiname="Output Height";> = 1080.0;*/

cbuffer controls:register(b0){
float InputWidth <string uiname="Input Width";> = 1920.0;
float InputHeight <string uiname="Input Height";> = 1080.0;
float OutputWidth <string uiname="Output Width";> = 1920.0;
float OutputHeight <string uiname="Output Height";> = 1080.0;
	
//float3x3 YUV2RGBTransform <string uiname="rgb2yuv matrix";>;
	
};

struct psInput
{
	float4 p : SV_Position;
	float2 uv : TEXCOORD0;
};

psInput VShaderSimple(float3 position : POSITION)
{
	psInput Out;
	Out.p = float4(position,1.0);
	Out.uv = 1.0;
}

psInput VShader(float3 position : POSITION, float2 uv:TEXCOORD)
{
	psInput Out;
	Out.p = float4(position,1.0);
	Out.uv = uv;
	return Out;
}


// --------------------------------------------------------------------------------------------------
// PIXELSHADERS:
// --------------------------------------------------------------------------------------------------

float2 toOutputIndex(float2 textureCoord)
{
	float2 R = float2(OutputWidth, OutputHeight);
	float2 pixelCoord = textureCoord * R;
	return pixelCoord;
}

float2 toInputCoord(float2 pixelCoord)
{
	float2 R = float2(InputWidth, InputHeight);
	return pixelCoord / R;
}

static float3 offset = {0.0625, 0.5, 0.5};
static float3 ycoeff = {0.256816, 0.504154, 0.0979137};
static float3 ucoeff = {-0.148246, -0.29102, 0.439266};
static float3 vcoeff = {0.439271, -0.367833, -0.071438};
static float4x4 yuvmatrix = {float4(ycoeff,0.0),float4(ucoeff,0.0),float4(vcoeff,0.0),float4(offset,1.0)};

float Y(float3 RGB)
{
	float value = dot(RGB,ycoeff);
	return saturate(value + offset.x);
}

float U(float3 RGB)
{
	float value = dot(RGB,ucoeff);
	return saturate(value + offset.y);
}

float V(float3 RGB)
{
	float value = dot(RGB,vcoeff);
	return saturate(value + offset.z);
}

float3 YUV(float3 RGB)
{
	return float3(Y(RGB), U(RGB), V(RGB));
}

float4 PSPassthrough(psInput In):SV_Target
{
	return tex0.SampleLevel(s0,In.uv,0);
}

float4 SwapRB(psInput In):SV_Target
{
	return tex0.SampleLevel(s0,In.uv,0).bgra;
}

float3 PSRGBA8888_to_YUV444_8(psInput In):SV_Target
{
	float3 rgb = tex0.SampleLevel(s0,In.uv,0).rgb;
	
	float y = Y(rgb);
	float u = U(rgb);
	float v = V(rgb);
	
	return float3(y, u, v);
}

float4 PSRGBA8888_to_YUV_ALPHA(psInput In):SV_Target
{
	float4 rgba = tex0.SampleLevel(s0,In.uv,0);
	
	float y = Y(rgba.rgb);
	float u = U(rgba.rgb);
	float v = V(rgba.rgb);
	
	return float4(y, u, v, rgba.a);
}

float4 PSRGBA8888_to_VUYA_4444(psInput In):SV_Target
{
	float4 rgba = tex0.SampleLevel(s0,In.uv,0);
	
	float y = Y(rgba.rgb);
	float u = U(rgba.rgb);
	float v = V(rgba.rgb);
	
	return float4(v, u, y, rgba.a);
}

float4 PSRGBA8888_to_YUVS(psInput In):SV_Target
{
	float3 rgbA = tex0.SampleLevel(s0, float2(In.uv.x, In.uv.y), 0).rgb;
	float3 rgbB = tex0.SampleLevel(s0, float2(In.uv.x + 1.0/InputWidth, In.uv.y), 0).rgb;
	
	float3 yuvA = YUV(rgbA);
	float3 yuvB = YUV(rgbB);
	
	float y1 = Y(rgbA);
	float y2 = Y(rgbB);
	float u = U(rgbA);
	float v = V(rgbA);

	return float4(y1, u, y2, v);
}

float4 PSRGBA8888_to_2VUY(psInput In):SV_Target
{
	float3 rgbA = tex0.SampleLevel(s0, float2(In.uv.x, In.uv.y), 0).rgb;
	float3 rgbB = tex0.SampleLevel(s0, float2(In.uv.x + 1.0/InputWidth, In.uv.y), 0).rgb;
	
	float3 yuvA = YUV(rgbA);
	float3 yuvB = YUV(rgbB);
	
	float y1 = Y(rgbA);
	float y2 = Y(rgbB);
	float u = U(rgbA);
	float v = V(rgbA);

	return float4(u, y1, v, y2);
}

float4 PSRGBA8888_to_RGB888(psInput In):SV_Target
{
	float x = In.uv.x * OutputWidth;
	float y = In.uv.y;
	uint begin = ((uint)x) % (uint)3;
	uint offset = ((uint)x) / (uint)3;
	x = (x + (float)offset) / InputWidth;
	float onePixel = 1.0/InputWidth;
	float4 rgba;
	if(begin==0)
	{
		rgba.rgb = tex0.SampleLevel(s0,float2(x,y),0).rgb;
		rgba.a = tex0.SampleLevel(s0,float2(x+onePixel,y),0).r;
	}
	else if(begin==1)
	{
		rgba.rg = tex0.SampleLevel(s0,float2(x,y),0).gb;
		rgba.ba = tex0.SampleLevel(s0,float2(x+onePixel,y),0).rg;
	}
	else
	{
		rgba.r = tex0.SampleLevel(s0,float2(x,y),0).b;
		rgba.gba = tex0.SampleLevel(s0,float2(x+onePixel,y),0).rgb;
	}
	
	return rgba;
}

float4 PSRGBA8888_to_BGR888(psInput In):SV_Target
{
	float x = In.uv.x * OutputWidth;
	float y = In.uv.y;
	uint begin = ((uint)x) % (uint)3;
	uint offset = ((uint)x) / (uint)3;
	x = (x + (float)offset) / InputWidth;
	float onePixel = 1.0/InputWidth;
	float4 rgba;
	if(begin==0)
	{
		rgba.rgb = tex0.SampleLevel(s0,float2(x,y),0).bgr;
		rgba.a = tex0.SampleLevel(s0,float2(x+onePixel,y),0).b;
	}
	else if(begin==1)
	{
		rgba.rg = tex0.SampleLevel(s0,float2(x,y),0).gr;
		rgba.ba = tex0.SampleLevel(s0,float2(x+onePixel,y),0).bg;
	}
	else
	{
		rgba.r = tex0.SampleLevel(s0,float2(x,y),0).r;
		rgba.gba = tex0.SampleLevel(s0,float2(x+onePixel,y),0).bgr;
	}
	
	return rgba;
}

float4 PSRGBA8888_to_V210(psInput In):SV_Target
{
	float x = In.uv.x * OutputWidth;
	float y = In.uv.y;
	uint begin = ((uint)x) % (uint)4;
	uint offset = ((uint)x) / (uint)4 * (uint)2;
	x = (x + (float)offset) / InputWidth;
	float onePixel = 1.0/InputWidth;
	float r;
	float g;
	float b;
	if(begin==0)
	{
		r = U(tex0.SampleLevel(s0,float2(x,y),0).rgb);
		g = Y(tex0.SampleLevel(s0,float2(x,y),0).rgb);
		b = V(tex0.SampleLevel(s0,float2(x,y),0).rgb);
	}
	else if(begin==1)
	{
		r = Y(tex0.SampleLevel(s0,float2(x,y),0).rgb);
		g = U(tex0.SampleLevel(s0,float2(x+onePixel,y),0).rgb);
		b = Y(tex0.SampleLevel(s0,float2(x+onePixel,y),0).rgb);
	}
	else if(begin==2)
	{
		r = V(tex0.SampleLevel(s0,float2(x,y),0).rgb);
		g = Y(tex0.SampleLevel(s0,float2(x+onePixel,y),0).rgb);
		b = U(tex0.SampleLevel(s0,float2(x+onePixel+onePixel,y),0).rgb);
	}
	else
	{
		r = Y(tex0.SampleLevel(s0,float2(x+onePixel,y),0).rgb);
		g = V(tex0.SampleLevel(s0,float2(x+onePixel,y),0).rgb);
		b = Y(tex0.SampleLevel(s0,float2(x+onePixel+onePixel,y),0).rgb);
	}

	return float4(r,g,b,0.0);
}

float4 PSRGBA8888_to_Y210(psInput In):SV_Target
{
	float x = In.uv.x * OutputWidth;
	float y = In.uv.y;
	uint begin = ((uint)x) % (uint)4;
	uint offset = ((uint)x) / (uint)4 * (uint)2;
	x = (x + (float)offset) / InputWidth;
	float onePixel = 1.0/InputWidth;
	float r;
	float g;
	float b;
	if(begin==0)
	{
		r = Y(tex0.SampleLevel(s0,float2(x,y),0).rgb);
		g = U(tex0.SampleLevel(s0,float2(x,y),0).rgb);
		b = Y(tex0.SampleLevel(s0,float2(x+onePixel,y),0).rgb);
	}
	else if(begin==1)
	{
		r = V(tex0.SampleLevel(s0,float2(x,y),0).rgb);
		g = Y(tex0.SampleLevel(s0,float2(x+onePixel,y),0).rgb);
		b = U(tex0.SampleLevel(s0,float2(x,y),0).rgb);
	}
	else if(begin==2)
	{
		r = Y(tex0.SampleLevel(s0,float2(x+onePixel,y),0).rgb);
		g = V(tex0.SampleLevel(s0,float2(x,y),0).rgb);
		b = Y(tex0.SampleLevel(s0,float2(x+onePixel+onePixel,y),0).rgb);
	}
	else
	{
		r = U(tex0.SampleLevel(s0,float2(x,y),0).rgb);
		g = Y(tex0.SampleLevel(s0,float2(x+onePixel+onePixel,y),0).rgb);
		b = V(tex0.SampleLevel(s0,float2(x,y),0).rgb);
	}

	return float4(r,g,b,0.0);
}

/*float4 PSYUV444_to_RGB888_8(psInput In):SV_Target
{
	float3 yuv = tex0.SampleLevel(s0,In.uv,0).rgb;
	yuv[0] -= 16.0f / 255.0f;
	
	float3 rgb = mul(yuv, YUV2RGBTransform);
	
	return float4(rgb.r, rgb.g, rgb.b, 1.0f);
}

float4 PSYUV422_to_RGB888_8(psInput In):SV_Target
{
	int2 outputIndex = (int2) (toOutputIndex(In.uv) + float2(0,0));
	int2 inputIndex = outputIndex;
	inputIndex.x /= (uint) 2;
	float2 inputCoord = toInputCoord((float2)inputIndex);
	
	float4 yuv2 = tex0.SampleLevel(s0, inputCoord, 0).rgba;
	
	bool rightPixel = outputIndex.x % (uint) 2;
	
	float y = rightPixel ? yuv2.a : yuv2.g;
	float u = yuv2.b - 0.5f;
	float v = yuv2.r - 0.5f;
	
	y -= 16.0f / 255.0f;
	
	float3 rgb = saturate(mul(float3(y,u,v), YUV2RGBTransform));
	return float4(rgb.r, rgb.g, rgb.b, 1.0f);
}*/

float4 PSNotSupported(psInput In):SV_Target
{
	float2 outputIndex = toOutputIndex(In.uv);
	outputIndex /= 10.0f;
	return sin(outputIndex.x + outputIndex.y);
}


/*technique10 Passthrough
{
	pass P0 {SetPixelShader(CompileShader(ps_4_0,PSPassthrough()));}
}

technique10 RGB888_to_YUV444_8
{
	pass P0 {SetPixelShader(CompileShader(ps_4_0,PSRGB888_to_YUV444_8()));}
}

technique10 RGB888_to_YUV422_8
{
	pass P0 {SetPixelShader(CompileShader(ps_4_0,PSRGB888_to_YUV422_8()));}
}

technique10 YUV444_to_RGB888_8
{
	pass P0 {SetPixelShader(CompileShader(ps_4_0,PSYUV444_to_RGB888_8()));}
}

technique10 YUV422_to_RGB888_8
{
	pass P0 {SetPixelShader(CompileShader(ps_4_0,PSYUV422_to_RGB888_8()));}
}

technique10 NotSupported
{
	pass P0 {SetPixelShader(CompileShader(ps_4_0,PSNotSupported()));}
}*/