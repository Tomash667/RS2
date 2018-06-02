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
cbuffer ps_globals
{
	float4 tint;
}

float4 ps_main(VS_OUTPUT In) : SV_TARGET
{
	return tint;
}
