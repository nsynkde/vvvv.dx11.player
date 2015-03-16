#include "HeadersPixelS.hlsl"
Texture2D<uint> tex0;

float4 A2R10G10B10_to_R10G10B10A2(psInput In):SV_Target
{
	float x = In.uv.x * OutputWidth;
	float y = In.uv.y * OutputHeight;
	return D3DX_R10G10B10A2_UNORM_to_FLOAT4(tex0.Load(int3(x,y,0)));
}