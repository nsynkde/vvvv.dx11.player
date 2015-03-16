#include "HeadersPixelS.hlsl"
Texture2D tex0;
float4 PSRGB888_to_RGBA8888(psInput In):SV_Target
{
	float x = In.uv.x * OutputWidth;
	float y = In.uv.y;
	uint begin = ((uint)x) % (uint)4;
	uint offset = ((uint)x) / (uint)4;
	x = (x -(float)offset) / InputWidth;
	float4 rgba;
	if(begin==0)
	{
		rgba.rgb = tex0.SampleLevel(s0,float2(x,y),0).rgb;
		rgba.a = 1.0;
	}
	else if(begin==1)
	{
		rgba.r = tex0.SampleLevel(s0,float2(x-OnePixelX,y),0).a;
		rgba.gb = tex0.SampleLevel(s0,float2(x,y),0).rg;
		rgba.a = 1.0;
	}
	else if(begin==2)
	{
		rgba.rg = tex0.SampleLevel(s0,float2(x-OnePixelX,y),0).ba;
		rgba.b = tex0.SampleLevel(s0,float2(x,y),0).r;
		rgba.a = 1.0;
	}
	else
	{
		rgba.rgb = tex0.SampleLevel(s0,float2(x-OnePixelX,y),0).gba;
		rgba.a = 1.0;
	}
	return rgba;
}