#include "HeadersPixelS.hlsl"
Texture2D tex0;
float4 main(psInput In):SV_Target
{
	uint x = In.uv.x * (InputWidth-Remainder);
	uint y = (YOrigin+YCoordinateSign*In.uv.y) * OutputHeight;
	uint pixelIdx = x + y * InputWidth - 1;
	uint totalWidth = InputWidth + RowPadding;
	int r_x = pixelIdx % totalWidth;
	int r_y = pixelIdx / totalWidth;
	int3 r = int3(r_x, r_y, 0);
	int g_x = (pixelIdx+1) % totalWidth;
	int g_y = (pixelIdx+1) / totalWidth;
	int3 g = int3(g_x, g_y, 0);
	int b_x = (pixelIdx + 2) % totalWidth;
	int b_y = (pixelIdx + 2) / totalWidth;
	int3 b = int3(b_x, b_y, 0);

	if (RequiresSwap > 0.5){
		return float4(tex0.Load(r).r, tex0.Load(g).r, tex0.Load(b).r, 1.0).bgra;
	}else{
		return float4(tex0.Load(r).r, tex0.Load(g).r, tex0.Load(b).r, 1.0);
	}
}