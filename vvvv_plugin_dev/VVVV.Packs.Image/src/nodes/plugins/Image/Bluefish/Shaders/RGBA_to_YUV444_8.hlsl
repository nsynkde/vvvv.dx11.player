#include "HeadersPixelS.hlsl"

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