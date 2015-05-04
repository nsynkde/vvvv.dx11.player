#include "HeadersPixelS.hlsl"

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