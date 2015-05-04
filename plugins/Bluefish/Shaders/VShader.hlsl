struct psInput
{
	float4 p : SV_Position;
	float2 uv : TEXCOORD0;
};

psInput VShaderSimple(float3 position : POSITION)
{
	psInput Out;
	Out.p = float4(position,1.0);
	Out.uv = 1.0;
}

psInput VShader(float3 position : POSITION, float2 uv:TEXCOORD)
{
	psInput Out;
	Out.p = float4(position,1.0);
	Out.uv = uv;
	return Out;
}