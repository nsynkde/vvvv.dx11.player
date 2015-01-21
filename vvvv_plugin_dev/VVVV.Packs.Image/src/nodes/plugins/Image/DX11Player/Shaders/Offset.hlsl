Texture2D<float4> BufferIn;
RWTexture2D<float4> BufferOut;

cbuffer controls:register(b0){
	int InputWidth <string uiname="Input Width";> = 3840;
	int Offset <string uiname="Offset";> = 128;
};

void writeToPixel(uint x, uint y, float4 colour)
{    
    BufferOut[uint2(x,y)] = colour.rgba;
}


float4 readPixel(uint x, uint y){
	return BufferIn.Load(uint3(x,y,0));
}

[numthreads(32, 30, 1)]
void CSOffset( uint3 DTid : SV_DispatchThreadID )
{
	uint idx = DTid.x + DTid.y * InputWidth;
	idx += Offset;
	uint x = idx % InputWidth;
	uint y = idx / InputWidth;
    writeToPixel(DTid.x,DTid.y,readPixel(x,y));
}