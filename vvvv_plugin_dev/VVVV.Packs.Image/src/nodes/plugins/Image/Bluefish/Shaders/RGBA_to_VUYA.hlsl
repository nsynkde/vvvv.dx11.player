#include "HeadersPixelS.hlsl"

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