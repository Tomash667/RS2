cbuffer vs_globals
{
	matrix mat_combined;
	matrix mat_bones[32];
};

struct VS_INPUT
{
    float3 pos : POSITION;
	half weight : BLENDWEIGHT0;
	uint4 indices : BLENDINDICES0;
	float3 normal : NORMAL;
	float2 tex : TEXCOORD0;
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
};

VS_OUTPUT vs_main(VS_INPUT In)
{
	VS_OUTPUT Out;
		
	float3 pos = (mul(float4(In.pos,1), mat_bones[In.indices[0]]) * In.weight).xyz;
	pos += (mul(float4(In.pos,1), mat_bones[In.indices[1]]) * (1-In.weight)).xyz;
	Out.pos = mul(float4(pos,1), mat_combined);
	Out.tex = In.tex;
	
	return Out;
}

//------------------------------------------------------------------
cbuffer ps_globals
{
	float4 fog_color;
	float4 fog_params;
	float4 tint;
};

Texture2D texture0;
SamplerState sampler0;

float4 ps_main(VS_OUTPUT In) : SV_TARGET
{
	float4 tex = texture0.Sample(sampler0, In.tex) * tint;
	float fog = saturate((In.pos.w - fog_params.x) / fog_params.z);
	return float4(lerp(tex.xyz, fog_color.xyz, fog), tex.w);
	return tex;
}
