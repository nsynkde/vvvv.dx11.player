Texture2D tex0;
SamplerState s0;

struct VOut
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};


float4 PShaderNoTex(VOut In):SV_Target
{
	return float4(1.0,1.0,1.0,1.0);
}