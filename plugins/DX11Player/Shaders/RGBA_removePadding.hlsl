#include "HeadersPixelS.hlsl"
Texture2D tex0;
float4 PSRGBA8888_remove_padding(psInput In):SV_Target
{
	uint x = In.uv.x * OutputWidth;
	uint y = (YOrigin+YCoordinateSign*In.uv.y) * OutputHeight;
	uint pixelIdx = x + y * OutputWidth;
	uint totalWidth = InputWidth + RowPadding;
	int in_x = pixelIdx % totalWidth;
	int in_y = pixelIdx / totalWidth;
	int3 uv = int3(in_x,in_y,0);
	return tex0.Load(uv).rgba;
}