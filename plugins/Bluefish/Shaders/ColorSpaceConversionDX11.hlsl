#include "HeadersPixelS.hlsl"

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


float3 PSRGBA8888_to_YUV444_8(psInput In):SV_Target
{
#if SWAP_RB
	float3 rgb = tex0.SampleLevel(s0,In.uv,0).bgr;
#else
	float3 rgb = tex0.SampleLevel(s0,In.uv,0).rgb;
#endif

	float y = Y(rgb);
	float u = U(rgb);
	float v = V(rgb);
	
	return float3(y, u, v);
}

float4 PSRGBA8888_to_YUV_ALPHA(psInput In):SV_Target
{
#if SWAP_RB
	float4 rgba = tex0.SampleLevel(s0,In.uv,0).bgra;
#else
	float4 rgba = tex0.SampleLevel(s0,In.uv,0);
#endif

	float y = Y(rgba.rgb);
	float u = U(rgba.rgb);
	float v = V(rgba.rgb);
	
	return float4(y, u, v, rgba.a);
}

float4 PSRGBA8888_to_VUYA_4444(psInput In):SV_Target
{
#if SWAP_RB
	float4 rgba = tex0.SampleLevel(s0,In.uv,0).bgra;
#else
	float4 rgba = tex0.SampleLevel(s0,In.uv,0);
#endif
	
	float y = Y(rgba.rgb);
	float u = U(rgba.rgb);
	float v = V(rgba.rgb);
	
	return float4(v, u, y, rgba.a);
}

float4 PSRGBA8888_to_YUVS(psInput In):SV_Target
{
#if SWAP_RB
	float3 rgbA = tex0.SampleLevel(s0, float2(In.uv.x, In.uv.y), 0).bgr;
	float3 rgbB = tex0.SampleLevel(s0, float2(In.uv.x + OnePixelX, In.uv.y), 0).bgr;
#else
	float3 rgbA = tex0.SampleLevel(s0, float2(In.uv.x, In.uv.y), 0).rgb;
	float3 rgbB = tex0.SampleLevel(s0, float2(In.uv.x + OnePixelX, In.uv.y), 0).rgb;
#endif
	
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
#if SWAP_RB
	float3 rgbA = tex0.SampleLevel(s0, float2(In.uv.x, In.uv.y), 0).bgr;
	float3 rgbB = tex0.SampleLevel(s0, float2(In.uv.x + OnePixelX, In.uv.y), 0).bgr;
#else
	float3 rgbA = tex0.SampleLevel(s0, float2(In.uv.x, In.uv.y), 0).rgb;
	float3 rgbB = tex0.SampleLevel(s0, float2(In.uv.x + OnePixelX, In.uv.y), 0).rgb;
#endif
	
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
	float4 rgba;
	if(begin==0)
	{
		rgba.rgb = tex0.SampleLevel(s0,float2(x,y),0).rgb;
		rgba.a = tex0.SampleLevel(s0,float2(x+OnePixelX,y),0).r;
	}
	else if(begin==1)
	{
		rgba.rg = tex0.SampleLevel(s0,float2(x,y),0).gb;
		rgba.ba = tex0.SampleLevel(s0,float2(x+OnePixelX,y),0).rg;
	}
	else
	{
		rgba.r = tex0.SampleLevel(s0,float2(x,y),0).b;
		rgba.gba = tex0.SampleLevel(s0,float2(x+OnePixelX,y),0).rgb;
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
	float4 rgba;
	if(begin==0)
	{
		rgba.rgb = tex0.SampleLevel(s0,float2(x,y),0).bgr;
		rgba.a = tex0.SampleLevel(s0,float2(x+OnePixelX,y),0).b;
	}
	else if(begin==1)
	{
		rgba.rg = tex0.SampleLevel(s0,float2(x,y),0).gr;
		rgba.ba = tex0.SampleLevel(s0,float2(x+OnePixelX,y),0).bg;
	}
	else
	{
		rgba.r = tex0.SampleLevel(s0,float2(x,y),0).r;
		rgba.gba = tex0.SampleLevel(s0,float2(x+OnePixelX,y),0).bgr;
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
	float r;
	float g;
	float b;
#if SWAP_RB
	float3 rgb0 = tex0.SampleLevel(s0, float2(x,y), 0).bgr;
	float3 rgb1 = tex0.SampleLevel(s0, float2(x+OnePixelX,y), 0).bgr;
	float3 rgb2 = tex0.SampleLevel(s0, float2(x+OnePixelX+OnePixelX,y), 0).bgr;
#else
	float3 rgb0 = tex0.SampleLevel(s0, float2(x,y), 0).rgb;
	float3 rgb1 = tex0.SampleLevel(s0, float2(x+OnePixelX,y), 0).rgb;
	float3 rgb2 = tex0.SampleLevel(s0, float2(x+OnePixelX+OnePixelX,y), 0).rgb;
#endif
	if(begin==0)
	{
		r = U(rgb0);
		g = Y(rgb0);
		b = V(rgb0);
	}
	else if(begin==1)
	{
		r = Y(rgb0);
		g = U(rgb1);
		b = Y(rgb1);
	}
	else if(begin==2)
	{
		r = V(rgb0);
		g = Y(rgb1);
		b = U(rgb2);
	}
	else
	{
		r = Y(rgb1);
		g = V(rgb1);
		b = Y(rgb2);
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
	float r;
	float g;
	float b;
#if SWAP_RB
	float3 rgb0 = tex0.SampleLevel(s0, float2(x,y), 0).bgr;
	float3 rgb1 = tex0.SampleLevel(s0, float2(x+OnePixelX,y), 0).bgr;
	float3 rgb2 = tex0.SampleLevel(s0, float2(x+OnePixelX+OnePixelX,y), 0).bgr;
#else
	float3 rgb0 = tex0.SampleLevel(s0, float2(x,y), 0).rgb;
	float3 rgb1 = tex0.SampleLevel(s0, float2(x+OnePixelX,y), 0).rgb;
	float3 rgb2 = tex0.SampleLevel(s0, float2(x+OnePixelX+OnePixelX,y), 0).rgb;
#endif
	if(begin==0)
	{
		r = Y(rgb0);
		g = U(rgb0);
		b = Y(rgb1);
	}
	else if(begin==1)
	{
		r = V(rgb0);
		g = Y(rgb1);
		b = U(rgb0);
	}
	else if(begin==2)
	{
		r = Y(rgb1);
		g = V(rgb0);
		b = Y(rgb2);
	}
	else
	{
		r = U(rgb0);
		g = Y(rgb2);
		b = V(rgb0);
	}

	return float4(r,g,b,0.0);
}

float4 PSNotSupported(psInput In):SV_Target
{
	float2 outputIndex = toOutputIndex(In.uv);
	outputIndex /= 10.0f;
	return sin(outputIndex.x + outputIndex.y);
}
