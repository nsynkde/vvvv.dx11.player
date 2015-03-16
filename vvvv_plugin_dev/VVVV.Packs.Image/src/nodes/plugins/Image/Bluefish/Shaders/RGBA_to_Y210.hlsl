#include "HeadersPixelS.hlsl"

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
