//@author: vux
//@help: template for texture fx
//@tags: texture
//@credits: 

Texture2D<float4> BufferIn;
//SamplerState s0;
RWStructuredBuffer<int> BufferOut : register(u0);


cbuffer controls:register(b0){
	float InputWidth <string uiname="Input Width";> = 1920.0;
	float InputHeight <string uiname="Input Height";> = 1080.0;
	float OutputWidth <string uiname="Output Width";> = 1920.0;
	float OutputHeight <string uiname="Output Height";> = 1080.0;
};


// --------------------------------------------------------------------------------------------------
// COMPUTE SHADERS:
// --------------------------------------------------------------------------------------------------


static float3 offset = {0.0625, 0.5, 0.5};
static float3 ycoeff = {0.256816, 0.504154, 0.0979137};
static float3 ucoeff = {-0.148246, -0.29102, 0.439266};
static float3 vcoeff = {0.439271, -0.367833, -0.071438};
static float4x4 yuvmatrix = {float4(ycoeff,0.0),float4(ucoeff,0.0),float4(vcoeff,0.0),float4(offset,1.0)};

float Y(float3 RGB)
{
	float value = dot(RGB,ycoeff);
	return saturate(value + offset.x);
}

float U(float3 RGB)
{
	float value = dot(RGB,ucoeff);
	return saturate(value + offset.y);
}

float V(float3 RGB)
{
	float value = dot(RGB,vcoeff);
	return saturate(value + offset.z);
}

float3 YUV(float3 RGB)
{
	return float3(Y(RGB), U(RGB), V(RGB));
}

float4 readPixel(uint x, uint y){
	return BufferIn.Load(uint3(x,y,0));
}

void writeToPixel(uint x, uint y, float4 colour)
{
    uint index = (x + y * (uint)OutputWidth);
    
    int ired   = ((int)(colour.r * 255.0));
    int igreen = ((int)(colour.g * 255.0)) << 8;
    int iblue  = ((int)(colour.b * 255.0)) << 16;
    int ialpha = ((int)(colour.a * 255.0)) << 24;
    
    BufferOut[index] = ired + igreen + iblue + ialpha;
}

void writeToPixel10(uint x, uint y, float4 colour)
{
    uint index = (x + y * (uint)OutputWidth);
    
    int ired   = ((int)(colour.r * 1023.0));
    int igreen = ((int)(colour.g * 1023.0)) << 10;
    int iblue  = ((int)(colour.b * 1023.0)) << 20;
    //int ialpha = 3 << 30;
    
    BufferOut[index] = ired + igreen + iblue;
}

[numthreads(32, 30, 1)]
void PSPassthrough( uint3 DTid : SV_DispatchThreadID )
{
    writeToPixel(DTid.x,DTid.y,readPixel(DTid.x,DTid.y));
}


[numthreads(32, 30, 1)]
void PSRGBA8888_to_VUYA_4444( uint3 DTid : SV_DispatchThreadID )
{
#if SWAP_RB
	float4 rgba = readPixel(DTid.x,DTid.y).bgra;
#else
	float4 rgba = readPixel(DTid.x,DTid.y);
#endif
	
	float y = Y(rgba.rgb);
	float u = U(rgba.rgb);
	float v = V(rgba.rgb);
	
	writeToPixel(DTid.x,DTid.y,float4(v, u, y, rgba.a));
}


[numthreads(16, 16, 1)]
void PSRGBA8888_to_YUVS( uint3 DTid : SV_DispatchThreadID )
{
#if SWAP_RB
	float3 rgbA = readPixel(DTid.x*2,DTid.y).bgr;
	float3 rgbB = readPixel(DTid.x*2+1,DTid.y).bgr;
#else
	float3 rgbA = readPixel(DTid.x*2,DTid.y).rgb;
	float3 rgbB = readPixel(DTid.x*2+1,DTid.y).rgb;
#endif
	
	float y1 = Y(rgbA);
	float y2 = Y(rgbB);
	float u = (U(rgbA) + U(rgbB)) / 2.0;
	float v = (V(rgbA) + V(rgbB)) / 2.0;

	writeToPixel(DTid.x,DTid.y,float4(y1, u, y2, v));
}

[numthreads(32, 30, 1)]
void PSRGBA8888_to_2VUY( uint3 DTid : SV_DispatchThreadID )
{
#if SWAP_RB
	float3 rgbA = readPixel(DTid.x*2,DTid.y).bgr;
	float3 rgbB = readPixel(DTid.x*2+1,DTid.y).bgr;
#else
	float3 rgbA = readPixel(DTid.x*2,DTid.y).rgb;
	float3 rgbB = readPixel(DTid.x*2+1,DTid.y).rgb;
#endif
	
	float y1 = Y(rgbA);
	float y2 = Y(rgbB);
	float u = U(rgbA);
	float v = V(rgbA);

	writeToPixel(DTid.x,DTid.y,float4(u, y1, v, y2));
}

[numthreads(32, 30, 1)]
void PSRGBA8888_to_RGB888( uint3 DTid : SV_DispatchThreadID )
{
	uint x = DTid.x;
	uint y = DTid.y;
	uint begin = x % 3;
	x = x * 4 / 3;
	float4 rgba;
	if(begin==0)
	{
		rgba.rgb = readPixel(x,y).rgb;
		rgba.a = readPixel(x+1,y).r;
	}
	else if(begin==1)
	{
		rgba.rg = readPixel(x,y).gb;
		rgba.ba = readPixel(x+1,y).rg;
	}
	else
	{
		rgba.r = readPixel(x,y).b;
		rgba.gba = readPixel(x+1,y).rgb;
	}
	
	writeToPixel(DTid.x,DTid.y,rgba);
}

[numthreads(32, 30, 1)]
void PSRGBA8888_to_BGR888( uint3 DTid : SV_DispatchThreadID )
{
	uint x = DTid.x;
	uint y = DTid.y;
	uint begin = x % 3;
	x = x * 4 / 3;
	float4 rgba;
	if(begin==0)
	{
		rgba.rgb = readPixel(x,y).bgr;
		rgba.a = readPixel(x+1,y).b;
	}
	else if(begin==1)
	{
		rgba.rg = readPixel(x,y).gr;
		rgba.ba = readPixel(x+1,y).bg;
	}
	else
	{
		rgba.r = readPixel(x,y).r;
		rgba.gba = readPixel(x+1,y).bgr;
	}
	
	writeToPixel(DTid.x,DTid.y,rgba);
}

[numthreads(32, 30, 1)]
void PSRGBA8888_to_V210( uint3 DTid : SV_DispatchThreadID )
{
	uint begin = DTid.x % 4;
	uint x = DTid.x * 6 / 4;
	uint y = DTid.y;
	float r;
	float g;
	float b;
	if(begin==0)
	{
		#if SWAP_RB
			float3 rgb0 = readPixel(x,y).bgr;
		#else
			float3 rgb0 = readPixel(x,y).rgb;
		#endif
		r = U(rgb0);
		g = Y(rgb0);
		b = V(rgb0);
	}
	else if(begin==1)
	{
		#if SWAP_RB
			float3 rgb0 = readPixel(x,y).bgr;
			float3 rgb1 = readPixel(x+1,y).bgr;
		#else
			float3 rgb0 = readPixel(x,y).rgb;
			float3 rgb1 = readPixel(x+1,y).rgb;
		#endif
		r = Y(rgb0);
		g = U(rgb1);
		b = Y(rgb1);
	}
	else if(begin==2)
	{
		#if SWAP_RB
			float3 rgb0 = readPixel(x-1,y).bgr;
			float3 rgb1 = readPixel(x,y).bgr;
			float3 rgb2 = readPixel(x+1,y).bgr;
		#else
			float3 rgb0 = readPixel(x-1,y).rgb;
			float3 rgb1 = readPixel(x,y).rgb;
			float3 rgb2 = readPixel(x+1,y).rgb;
		#endif
		r = V(rgb0);
		g = Y(rgb1);
		b = U(rgb2);
	}
	else
	{
		#if SWAP_RB
			float3 rgb1 = readPixel(x,y).bgr;
			float3 rgb2 = readPixel(x+1,y).bgr;
		#else
			float3 rgb1 = readPixel(x,y).rgb;
			float3 rgb2 = readPixel(x+1,y).rgb;
		#endif
		r = Y(rgb1);
		g = V(rgb1);
		b = Y(rgb2);
	}

	writeToPixel10(DTid.x,DTid.y,float4(r,g,b,0.0));
}

[numthreads(32, 30, 1)]
void PSRGBA8888_to_Y210( uint3 DTid : SV_DispatchThreadID )
{
	uint x = DTid.x;
#if SWAP_RB
	float3 rgbA = readPixel(x,DTid.y).bgr;
	float3 rgbB = readPixel(x+1,DTid.y).bgr;
#else
	float3 rgbA = readPixel(x,DTid.y).rgb;
	float3 rgbB = readPixel(x+1,DTid.y).rgb;
#endif
	
	if(DTid.x % 2 == 0){
		float y1 = Y(rgbA);
		float u = U(rgbA);
	
		uint index = (DTid.x + DTid.y * (uint)OutputWidth);
    
		int ired   = ((int)(y1 * 1023.0));
		int igreen = ((int)(u * 1023.0)) << 16;
    
		BufferOut[index] = ired + igreen;
	}else{
		float y2 = Y(rgbB);
		float v = V(rgbA);
	
		uint index = (DTid.x + DTid.y * (uint)OutputWidth);
    
		int ired   = ((int)(y2 * 1023.0));
		int igreen = ((int)(v * 1023.0)) << 16;
    
		BufferOut[index] = ired + igreen;
	}
}

/*float4 PSPassthrough(psInput In):SV_Target
{
	return tex0.SampleLevel(s0,In.uv,0);
}

float4 SwapRB(psInput In):SV_Target
{
	return tex0.SampleLevel(s0,In.uv,0).bgra;
}

float3 PSRGBA8888_to_YUV444_8(psInput In):SV_Target
{
#if SWAP_RB
	float3 rgb = tex0.SampleLevel(s0,In.uv,0).bgr;
#else
	float3 rgb = tex0.SampleLevel(s0,In.uv,0).rgb;
#endif

	float y = Y(rgb);
	float u = U(rgb);
	float v = V(rgb);
	
	return float3(y, u, v);
}

float4 PSRGBA8888_to_YUV_ALPHA(psInput In):SV_Target
{
#if SWAP_RB
	float4 rgba = tex0.SampleLevel(s0,In.uv,0).bgra;
#else
	float4 rgba = tex0.SampleLevel(s0,In.uv,0);
#endif

	float y = Y(rgba.rgb);
	float u = U(rgba.rgb);
	float v = V(rgba.rgb);
	
	return float4(y, u, v, rgba.a);
}

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

float4 PSRGBA8888_to_YUVS(psInput In):SV_Target
{
#if SWAP_RB
	float3 rgbA = tex0.SampleLevel(s0, float2(In.uv.x, In.uv.y), 0).bgr;
	float3 rgbB = tex0.SampleLevel(s0, float2(In.uv.x + OnePixelX, In.uv.y), 0).bgr;
#else
	float3 rgbA = tex0.SampleLevel(s0, float2(In.uv.x, In.uv.y), 0).rgb;
	float3 rgbB = tex0.SampleLevel(s0, float2(In.uv.x + OnePixelX, In.uv.y), 0).rgb;
#endif
	
	float3 yuvA = YUV(rgbA);
	float3 yuvB = YUV(rgbB);
	
	float y1 = Y(rgbA);
	float y2 = Y(rgbB);
	float u = U(rgbA);
	float v = V(rgbA);

	return float4(y1, u, y2, v);
}

float4 PSRGBA8888_to_2VUY(psInput In):SV_Target
{
#if SWAP_RB
	float3 rgbA = tex0.SampleLevel(s0, float2(In.uv.x, In.uv.y), 0).bgr;
	float3 rgbB = tex0.SampleLevel(s0, float2(In.uv.x + OnePixelX, In.uv.y), 0).bgr;
#else
	float3 rgbA = tex0.SampleLevel(s0, float2(In.uv.x, In.uv.y), 0).rgb;
	float3 rgbB = tex0.SampleLevel(s0, float2(In.uv.x + OnePixelX, In.uv.y), 0).rgb;
#endif
	
	float3 yuvA = YUV(rgbA);
	float3 yuvB = YUV(rgbB);
	
	float y1 = Y(rgbA);
	float y2 = Y(rgbB);
	float u = U(rgbA);
	float v = V(rgbA);

	return float4(u, y1, v, y2);
}

float4 PSRGBA8888_to_RGB888(psInput In):SV_Target
{
	float x = In.uv.x * OutputWidth;
	float y = In.uv.y;
	uint begin = ((uint)x) % (uint)3;
	uint offset = ((uint)x) / (uint)3;
	x = (x + (float)offset) / InputWidth;
	float4 rgba;
	if(begin==0)
	{
		rgba.rgb = tex0.SampleLevel(s0,float2(x,y),0).rgb;
		rgba.a = tex0.SampleLevel(s0,float2(x+OnePixelX,y),0).r;
	}
	else if(begin==1)
	{
		rgba.rg = tex0.SampleLevel(s0,float2(x,y),0).gb;
		rgba.ba = tex0.SampleLevel(s0,float2(x+OnePixelX,y),0).rg;
	}
	else
	{
		rgba.r = tex0.SampleLevel(s0,float2(x,y),0).b;
		rgba.gba = tex0.SampleLevel(s0,float2(x+OnePixelX,y),0).rgb;
	}
	
	return rgba;
}

float4 PSRGBA8888_to_BGR888(psInput In):SV_Target
{
	float x = In.uv.x * OutputWidth;
	float y = In.uv.y;
	uint begin = ((uint)x) % (uint)3;
	uint offset = ((uint)x) / (uint)3;
	x = (x + (float)offset) / InputWidth;
	float4 rgba;
	if(begin==0)
	{
		rgba.rgb = tex0.SampleLevel(s0,float2(x,y),0).bgr;
		rgba.a = tex0.SampleLevel(s0,float2(x+OnePixelX,y),0).b;
	}
	else if(begin==1)
	{
		rgba.rg = tex0.SampleLevel(s0,float2(x,y),0).gr;
		rgba.ba = tex0.SampleLevel(s0,float2(x+OnePixelX,y),0).bg;
	}
	else
	{
		rgba.r = tex0.SampleLevel(s0,float2(x,y),0).r;
		rgba.gba = tex0.SampleLevel(s0,float2(x+OnePixelX,y),0).bgr;
	}
	
	return rgba;
}

float4 PSRGBA8888_to_V210(psInput In):SV_Target
{
	float x = In.uv.x * OutputWidth;
	float y = In.uv.y;
	uint begin = ((uint)x) % (uint)4;
	uint offset = ((uint)x) / (uint)4 * (uint)2;
	x = (x + (float)offset) / InputWidth;
	float r;
	float g;
	float b;
#if SWAP_RB
	float3 rgb0 = tex0.SampleLevel(s0, float2(x,y), 0).bgr;
	float3 rgb1 = tex0.SampleLevel(s0, float2(x+OnePixelX,y), 0).bgr;
	float3 rgb2 = tex0.SampleLevel(s0, float2(x+OnePixelX+OnePixelX,y), 0).bgr;
#else
	float3 rgb0 = tex0.SampleLevel(s0, float2(x,y), 0).rgb;
	float3 rgb1 = tex0.SampleLevel(s0, float2(x+OnePixelX,y), 0).rgb;
	float3 rgb2 = tex0.SampleLevel(s0, float2(x+OnePixelX+OnePixelX,y), 0).rgb;
#endif
	if(begin==0)
	{
		r = U(rgb0);
		g = Y(rgb0);
		b = V(rgb0);
	}
	else if(begin==1)
	{
		r = Y(rgb0);
		g = U(rgb1);
		b = Y(rgb1);
	}
	else if(begin==2)
	{
		r = V(rgb0);
		g = Y(rgb1);
		b = U(rgb2);
	}
	else
	{
		r = Y(rgb1);
		g = V(rgb1);
		b = Y(rgb2);
	}

	return float4(r,g,b,0.0);
}

float4 PSRGBA8888_to_Y210(psInput In):SV_Target
{
	float x = In.uv.x * OutputWidth;
	float y = In.uv.y;
	uint begin = ((uint)x) % (uint)4;
	uint offset = ((uint)x) / (uint)4 * (uint)2;
	x = (x + (float)offset) / InputWidth;
	float r;
	float g;
	float b;
#if SWAP_RB
	float3 rgb0 = tex0.SampleLevel(s0, float2(x,y), 0).bgr;
	float3 rgb1 = tex0.SampleLevel(s0, float2(x+OnePixelX,y), 0).bgr;
	float3 rgb2 = tex0.SampleLevel(s0, float2(x+OnePixelX+OnePixelX,y), 0).bgr;
#else
	float3 rgb0 = tex0.SampleLevel(s0, float2(x,y), 0).rgb;
	float3 rgb1 = tex0.SampleLevel(s0, float2(x+OnePixelX,y), 0).rgb;
	float3 rgb2 = tex0.SampleLevel(s0, float2(x+OnePixelX+OnePixelX,y), 0).rgb;
#endif
	if(begin==0)
	{
		r = Y(rgb0);
		g = U(rgb0);
		b = Y(rgb1);
	}
	else if(begin==1)
	{
		r = V(rgb0);
		g = Y(rgb1);
		b = U(rgb0);
	}
	else if(begin==2)
	{
		r = Y(rgb1);
		g = V(rgb0);
		b = Y(rgb2);
	}
	else
	{
		r = U(rgb0);
		g = Y(rgb2);
		b = V(rgb0);
	}

	return float4(r,g,b,0.0);
}

float4 PSNotSupported(psInput In):SV_Target
{
	float2 outputIndex = toOutputIndex(In.uv);
	outputIndex /= 10.0f;
	return sin(outputIndex.x + outputIndex.y);
}*/
