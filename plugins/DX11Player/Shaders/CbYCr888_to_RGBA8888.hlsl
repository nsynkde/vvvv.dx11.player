#include "HeadersPixelS.hlsl"
Texture2D tex0;
float4 PSCbYCr888_to_RGBA8888(psInput In):SV_Target
{
	float x = In.uv.x * OutputWidth;
	float y = In.uv.y;
	uint begin = ((uint)x) % (uint)4;
	uint offset = ((uint)x) / (uint)4;
	x = (x -(float)offset) / InputWidth;
	float3 yuv;
	if(begin==0)
	{
		yuv = tex0.SampleLevel(s0,float2(x,y),0).rgb;
	}
	else if(begin==1)
	{
		yuv.r = tex0.SampleLevel(s0,float2(x-OnePixelX,y),0).a;
		yuv.gb = tex0.SampleLevel(s0,float2(x,y),0).rg;
	}
	else if(begin==2)
	{
		yuv.rg = tex0.SampleLevel(s0,float2(x-OnePixelX,y),0).ba;
		yuv.b = tex0.SampleLevel(s0,float2(x,y),0).r;
	}
	else
	{
		yuv.rgb = tex0.SampleLevel(s0,float2(x-OnePixelX,y),0).gba;
	}
	return float4(RGB_from_YUV(yuv.yxz),1.0);
}