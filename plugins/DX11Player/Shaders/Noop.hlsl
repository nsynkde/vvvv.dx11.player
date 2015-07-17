#include "HeadersPixelS.hlsl"
Texture2D tex0;
float4 main(psInput In) :SV_Target
{
	uint x = In.uv.x * InputWidth;
	uint y = (YOrigin + YCoordinateSign*In.uv.y) * OutputHeight;
	return tex0.Load(int3(x, y, 0));
}