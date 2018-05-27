cbuffer vs_globals : register(b0)
{
	matrix mat_combined;
	matrix mat_world;
};

cbuffer vs_animated_globals : register(b0)
{
	matrix mat_combined_ani;
	matrix mat_world_ani;
	matrix mat_bones[32];
};

struct VS_INPUT
{
    float3 pos : POSITION;
	float3 normal : NORMAL;
	float2 tex : TEXCOORD0;
};

struct VS_INPUT_ANIMATED
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
	float3 normal : TEXCOORD1;
};

VS_OUTPUT vs_mesh(VS_INPUT In)
{
	VS_OUTPUT Out;
	Out.pos = mul(float4(In.pos,1), mat_combined);
	Out.normal = mul(In.normal, (float3x3)mat_world).xyz;
	Out.tex = In.tex;
	return Out;
}

VS_OUTPUT vs_animated(VS_INPUT_ANIMATED In)
{
	VS_OUTPUT Out;
	float3 pos = (mul(float4(In.pos,1), mat_bones[In.indices[0]]) * In.weight).xyz;
	pos += (mul(float4(In.pos,1), mat_bones[In.indices[1]]) * (1-In.weight)).xyz;
	Out.pos = mul(float4(pos,1), mat_combined_ani);
	float3 normal = (mul(float4(In.normal,1), mat_bones[In.indices[0]]) * In.weight).xyz;
	normal += (mul(float4(In.normal,1), mat_bones[In.indices[1]]) * (1-In.weight)).xyz;
	Out.normal = mul(normal, (float3x3)mat_world_ani).xyz;
	Out.tex = In.tex;
	return Out;
}

//------------------------------------------------------------------
cbuffer ps_globals : register(b0)
{
	float3 fog_color;
	float3 fog_params;
	float3 light_dir;
	float3 light_color;
	float3 ambient_color;
};

cbuffer ps_per_object : register(b1)
{
	float4 tint;
}

Texture2D texture0;
SamplerState sampler0;

float4 ps_main(VS_OUTPUT In) : SV_TARGET
{
	float ndotl = dot(light_dir, In.normal);
	float3 diffuse = saturate(ndotl) * light_color;
	float4 tex = texture0.Sample(sampler0, In.tex) * tint;
	float4 final_color = float4(tex.xyz * saturate(diffuse + ambient_color), tex.w);
	float fog = saturate((In.pos.w - fog_params.x) / fog_params.z);
	return float4(lerp(final_color.xyz, fog_color, fog), final_color.w);
}
