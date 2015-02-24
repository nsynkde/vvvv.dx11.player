#include "HeadersPixelS.hlsl"
Texture2D tex0;

uint SwapEndian16(uint v){
	return ((v & 0xFF)<<8) | (((v>>8) & 0xFF));
}

float4 PSCbYCr161616_to_RGBA16161616(psInput In):SV_Target
{
	
	float x = In.uv.x * OutputWidth;
	float y = In.uv.y;
	uint begin = ((uint)x) % (uint)4;
	uint offset = ((uint)x) / (uint)4;
	x = (x -(float)offset) / InputWidth;
	float3 yuv;
	if(begin==0)
	{
		yuv = tex0.SampleLevel(s0,float2(x,y),0).rgb;
	}
	else if(begin==1)
	{
		yuv.r = tex0.SampleLevel(s0,float2(x-OnePixelX,y),0).a;
		yuv.gb = tex0.SampleLevel(s0,float2(x,y),0).rg;
	}
	else if(begin==2)
	{
		yuv.rg = tex0.SampleLevel(s0,float2(x-OnePixelX,y),0).ba;
		yuv.b = tex0.SampleLevel(s0,float2(x,y),0).r;
	}
	else
	{
		yuv.rgb = tex0.SampleLevel(s0,float2(x-OnePixelX,y),0).gba;
	}

	float Y = SwapEndian16(yuv.y*65535.0)/65535.0;
	float U = SwapEndian16(yuv.x*65535.0)/65535.0;
	float V = SwapEndian16(yuv.z*65535.0)/65535.0;

	return float4(RGB_from_YUV(float3(Y,U,V)),1.0);
}