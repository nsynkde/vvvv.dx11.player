
SamplerState s0;

float2 R:TARGETSIZE;

struct psInput
{
	float4 p : SV_Position;
	float2 uv : TEXCOORD0;
};


cbuffer controls:register(b0){
float InputWidth <string uiname="Input Width";> = 1920.0;
float InputHeight <string uiname="Input Height";> = 1080.0;
float OutputWidth <string uiname="Output Width";> = 1920.0;
float OutputHeight <string uiname="Output Height";> = 1080.0;
float OnePixelX = 1.0/1920.0;
float YOrigin = 0.0;
float YCoordinateSign = 1.0;
bool RequiresSwap = false;
	
//float3x3 YUV2RGBTransform <string uiname="rgb2yuv matrix";>;
	
};

float2 toOutputIndex(float2 textureCoord)
{
	float2 R = float2(OutputWidth, OutputHeight);
	float2 pixelCoord = textureCoord * R;
	return pixelCoord;
}

float2 toInputCoord(float2 pixelCoord)
{
	float2 R = float2(InputWidth, InputHeight);
	return pixelCoord / R;
}

static float3 offset = {0.0625, 0.5, 0.5};
static float3 ycoeff = {0.256816, 0.504154, 0.0979137};
static float3 ucoeff = {-0.148246, -0.29102, 0.439266};
static float3 vcoeff = {0.439271, -0.367833, -0.071438};

static float3 rgb_offset = {-0.0625, -0.5, -0.5};
static float3 rcoeff = {1.164, 0.000, 1.596};
static float3 gcoeff = {1.164,-0.391,-0.813};
static float3 bcoeff = {1.164, 2.018, 0.000};

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


float4 D3DX_R10G10B10A2_UNORM_to_FLOAT4(uint packedInput)  
{  
    float4 unpackedOutput;  
	if(RequiresSwap){
		packedInput = ((packedInput & 255)<<24) | (((packedInput>>8) & 255) << 16) | (((packedInput>>16) & 255) << 8) | ((packedInput>>24) & 255);
		unpackedOutput.r = (float)(((packedInput>>22)) & 1023)  / 1023;  
		unpackedOutput.g = (float)(((packedInput>>12)) & 1023) / 1023;  
		unpackedOutput.b = (float)(((packedInput>>2)) & 1023) / 1023;   
		unpackedOutput.a = 1.0;
	}else{
		unpackedOutput.r = (float)(((packedInput>>2)) & 1023)  / 1023;  
		unpackedOutput.g = (float)(((packedInput>>12)) & 1023) / 1023;  
		unpackedOutput.b = (float)(((packedInput>>22)) & 1023) / 1023;   
		unpackedOutput.a = 1.0;
	}
    return unpackedOutput;  
}  

float3 RGB_from_YUV(float3 YUV){
	YUV += rgb_offset;
	float r = dot(YUV, rcoeff);
	float g = dot(YUV, gcoeff);
	float b = dot(YUV, bcoeff);
	return float3(r,g,b);
}

/*float4 SwapRB(psInput In):SV_Target
{
	return tex0.SampleLevel(s0,In.uv,0).bgra;
}*/