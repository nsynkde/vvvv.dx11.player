#include "HeadersPixelS.hlsl"

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