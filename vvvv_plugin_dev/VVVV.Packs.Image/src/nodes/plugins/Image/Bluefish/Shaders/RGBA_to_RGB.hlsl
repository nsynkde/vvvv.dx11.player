#include "HeadersPixelS.hlsl"

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