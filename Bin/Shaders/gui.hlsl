cbuffer vs_globals
{
	float2 size;
};

struct VS_INPUT
{
    float2 pos : POSITION;
	float2 tex : TEXCOORD0;
	float4 color : COLOR0;
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
	float4 color : COLOR0;
};

VS_OUTPUT vs_main(VS_INPUT In)
{
	VS_OUTPUT Out;
	Out.pos.x = (In.pos.x / (size.x * 0.5f)) - 1.0f;
	Out.pos.y = -((In.pos.y / (size.y * 0.5f)) - 1.0f);
	Out.pos.z = 0.f;
	Out.pos.w = 1.f;
	Out.tex = In.tex;
	Out.color = In.color;
	return Out;
}

//------------------------------------------------------------------
Texture2D tex;
SamplerState sampler0;

float4 ps_main(VS_OUTPUT In) : SV_TARGET
{
	return tex.Sample(sampler0, In.tex) * In.color;
}
