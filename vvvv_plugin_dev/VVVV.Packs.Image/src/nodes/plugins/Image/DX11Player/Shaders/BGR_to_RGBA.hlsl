#include "HeadersPixelS.hlsl"
Texture2D tex0;
float4 PSBGR888_to_RGBA8888(psInput In):SV_Target
{
	float x = In.uv.x * OutputWidth;
	float y = YOrigin+YCoordinateSign*In.uv.y;
	uint begin = ((uint)x) % (uint)4;
	uint offset = ((uint)x) / (uint)4;
	x = (x -(float)offset) / InputWidth;
	float4 rgba;
	if(begin==0)
	{
		rgba.bgr = tex0.SampleLevel(s0,float2(x,y),0).rgb;
		rgba.a = 1.0;
	}
	else if(begin==1)
	{
		rgba.b = tex0.SampleLevel(s0,float2(x-OnePixelX,y),0).a;
		rgba.gr = tex0.SampleLevel(s0,float2(x,y),0).rg;
		rgba.a = 1.0;
	}
	else if(begin==2)
	{
		rgba.bg = tex0.SampleLevel(s0,float2(x-OnePixelX,y),0).ba;
		rgba.r = tex0.SampleLevel(s0,float2(x,y),0).r;
		rgba.a = 1.0;
	}
	else
	{
		rgba.bgr = tex0.SampleLevel(s0,float2(x-OnePixelX,y),0).gba;
		rgba.a = 1.0;
	}
	return rgba;
}