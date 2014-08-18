#include "HeadersPixelS.hlsl"

float4 PSPassthrough(psInput In):SV_Target
{
	return tex0.SampleLevel(s0,In.uv,0);
}