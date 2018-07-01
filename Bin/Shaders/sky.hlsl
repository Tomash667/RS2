//==============================================================================
// SKYDOME VS
cbuffer skydome_vs_globals
{
	matrix mat_combined;
}

struct skydome_vs_input
{
	float3 pos : POSITION;
	float2 tex : TEXCOORD0;
	float latitude : TEXCOORD1;
};

struct skydome_vs_output
{
    float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
	float latitude : TEXCOORD1;
};

skydome_vs_output skydome_vs(skydome_vs_input In)
{
	skydome_vs_output Out;
	Out.pos = mul(float4(In.pos, 1), mat_combined);
	Out.tex = In.tex;
	Out.latitude = In.latitude;
	return Out;
}

//==============================================================================
// SKYDOME PS
cbuffer skydome_ps_globals
{
	float4 horizon_color;
	float4 zenith_color;
	float stars_visibility;
}

Texture2D texture0;
SamplerState sampler0;

float4 skydome_ps(skydome_vs_output In) : SV_TARGET
{
	float4 texel = lerp(horizon_color, zenith_color, In.latitude);
	texel += texture0.Sample(sampler0, In.tex) * stars_visibility * In.latitude;
	return texel;
}

//==============================================================================
// CELESTIAL OBJECT VS
struct celestial_vs_input
{
	float3 pos : POSITION0;
	float2 tex : TEXCOORD0;
};

struct celestial_vs_output
{
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
};

celestial_vs_output celestial_vs(celestial_vs_input In)
{
	celestial_vs_output Out;
	Out.pos = mul(float4(In.pos, 1), mat_combined);
	Out.tex = In.tex;
	return Out;
}

//==============================================================================
// CELESTIAL OBJECT PS
cbuffer celestial_ps_globals
{
	float4 celestial_color;
}

float4 celestial_ps(celestial_vs_output In) : SV_TARGET
{
	return texture0.Sample(sampler0, In.tex) * celestial_color;
}

//==============================================================================
// CLOUDS VS
cbuffer clouds_vs_globals
{
	matrix mat_combined_clouds;
	float4 noiseTexA01;
	float4 noiseTexA23;
	float4 noiseTexB01;
	float4 noiseTexB23;
};

struct clouds_vs_output
{
	float4 pos : SV_POSITION;
	float latitude : TEXCOORD0;
	float2 noiseTex0 : TEXCOORD1;
	float2 noiseTex1 : TEXCOORD2;
	float2 noiseTex2 : TEXCOORD3;
	float2 noiseTex3 : TEXCOORD4;
};

clouds_vs_output clouds_vs(skydome_vs_input In)
{
	clouds_vs_output Out;
	Out.pos = mul(float4(In.pos, 1), mat_combined_clouds);
	Out.latitude = In.latitude;
	Out.noiseTex0 = In.tex * noiseTexA01.xy + noiseTexB01.xy;
	Out.noiseTex1 = In.tex * noiseTexA01.zw + noiseTexB01.zw;
	Out.noiseTex2 = In.tex * noiseTexA23.xy + noiseTexB23.xy;
	Out.noiseTex3 = In.tex * noiseTexA23.zw + noiseTexB23.zw;
	return Out;
}

//==============================================================================
// CLOUDS PS
cbuffer clouds_ps_globals
{
	float4 color1;
	float4 color2;
	float sharpness;
	float threshold;
}

float4 clouds_ps(clouds_vs_output In) : SV_TARGET
{
	float4 cloudSamples;
	cloudSamples.x = texture0.Sample(sampler0, In.noiseTex0).r;
	cloudSamples.y = texture0.Sample(sampler0, In.noiseTex1).r;
	cloudSamples.z = texture0.Sample(sampler0, In.noiseTex2).r;
	cloudSamples.w = texture0.Sample(sampler0, In.noiseTex3).r;
	float cloudSample = dot(cloudSamples, float4(1.0/1.0, 1.0/2.0, 1.0/4.0, 1.0/8.0));
	
	//float f_1 = 1 - exp( sharpness * (threshold-1) );
	float f_1 = 1;
	cloudSample = saturate(1 - exp(sharpness*threshold - sharpness*cloudSample)) / f_1;

	float4 texel = lerp(color1, color2, cloudSample);
	texel.a = cloudSample * In.latitude;
	return texel;
}
