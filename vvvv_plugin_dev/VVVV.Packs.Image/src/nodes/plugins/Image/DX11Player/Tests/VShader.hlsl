struct VOut
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

VOut VShader(float4 position : POSITION, float2 uv:TEXCOORD)
{
    VOut output;

    output.position = position;
    output.uv = uv;

    return output;
}
