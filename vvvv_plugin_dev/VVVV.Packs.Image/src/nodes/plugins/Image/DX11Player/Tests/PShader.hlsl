Texture2D tex0;
SamplerState s0;

struct VOut
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};


float4 PShader(VOut In):SV_Target
{
	return tex0.SampleLevel(s0,In.uv,0);
}