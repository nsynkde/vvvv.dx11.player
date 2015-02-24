#include "HeadersPixelS.hlsl"
Texture2D<uint> tex0;

float4 PSCbYCr101010_to_R10G10B10A2(psInput In):SV_Target
{
	uint x = In.uv.x * OutputWidth;
	uint y = In.uv.y * OutputHeight;
	float4 yuv = D3DX_R10G10B10A2_UNORM_to_FLOAT4(tex0.Load(int3(x,y,0)));
	return float4(RGB_from_YUV(yuv.yxz),1.0);
}