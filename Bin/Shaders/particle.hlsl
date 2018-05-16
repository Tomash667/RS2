cbuffer vs_globals
{
	matrix mat_combined;
};

struct VS_INPUT
{
	float3 pos : POSITION;
	float2 tex : TEXCOORD;
	float4 color : COLOR;
};

struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD;
	float4 color : COLOR;
};

VS_OUTPUT vs_main(VS_INPUT In)
{
	VS_OUTPUT Out;
	Out.pos = mul(float4(In.pos,1), mat_combined);
	Out.tex = In.tex;
	Out.color = In.color;
	return Out;
}

//------------------------------------------------------------------
Texture2D texture0;
SamplerState sampler0;

float4 ps_main(VS_OUTPUT In) : SV_TARGET
{
	return texture0.Sample(sampler0, In.tex) * In.color;
}
