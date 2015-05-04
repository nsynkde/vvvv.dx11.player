#include "HeadersPixelS.hlsl"
Texture2D tex0;
float4 PSBGR888_to_RGBA8888(psInput In):SV_Target
{
	uint x = In.uv.x * OutputWidth;
	uint y = (YOrigin+YCoordinateSign*In.uv.y) * OutputHeight;
	uint pixelIdx = x + y * OutputWidth;
	uint begin = pixelIdx % (uint)4;
	uint offset = pixelIdx / (uint)4;
	pixelIdx = pixelIdx - offset;
	uint totalWidth = InputWidth + RowPadding;
	int in_x = pixelIdx % totalWidth;
	int in_y = pixelIdx / totalWidth;
	int prev_x = (pixelIdx-1) % totalWidth;
	int prev_y = (pixelIdx-1) / totalWidth;
	int3 uv = int3(in_x,in_y,0);
	int3 prev_uv = int3(prev_x,prev_y,0);
	float4 rgba;
	if(begin==0)
	{
		rgba.bgr = tex0.Load(uv).rgb;
		rgba.a = 1.0;
	}
	else if(begin==1)
	{
		rgba.b = tex0.Load(prev_uv).a;
		rgba.gr = tex0.Load(uv).rg;
		rgba.a = 1.0;
	}
	else if(begin==2)
	{
		rgba.bg = tex0.Load(prev_uv).ba;
		rgba.r = tex0.Load(uv).r;
		rgba.a = 1.0;
	}
	else
	{
		rgba.bgr = tex0.Load(prev_uv).gba;
		rgba.a = 1.0;
	}
	return rgba;
}