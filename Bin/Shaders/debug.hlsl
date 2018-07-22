cbuffer vs_globals
{
	matrix mat_combined;
};

struct VS_INPUT
{
    float3 pos : POSITION;
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
};

VS_OUTPUT vs_main(VS_INPUT In)
{
	VS_OUTPUT Out;
	Out.pos = mul(float4(In.pos,1), mat_combined);
	return Out;
}

//------------------------------------------------------------------
struct VS_INPUT_COLOR
{
	float3 pos : POSITION;
	float4 color : COLOR0;
};

struct VS_OUTPUT_COLOR
{
	float4 pos : SV_POSITION;
	float4 color : COLOR0;
};

VS_OUTPUT_COLOR vs_main_color(VS_INPUT_COLOR In)
{
	VS_OUTPUT_COLOR Out;
	Out.pos = mul(float4(In.pos,1), mat_combined);
	Out.color = In.color;
	return Out;
}

//------------------------------------------------------------------
cbuffer ps_globals
{
	float4 tint;
}

float4 ps_main(VS_OUTPUT In) : SV_TARGET
{
	return tint;
}

//------------------------------------------------------------------
float4 ps_main_color(VS_OUTPUT_COLOR In) : SV_TARGET
{
	return In.color;
}
