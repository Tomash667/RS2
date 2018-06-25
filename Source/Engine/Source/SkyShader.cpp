#include "EngineCore.h"
#include "SkyShader.h"
#include "Sky.h"
#include "Render.h"
#include "Mesh.h"
#include "Vertex.h"
#include "Texture.h"
#include <d3d11_1.h>
#include "InternalHelper.h"

const uint SKYDOME_SLICES = 32;
const uint SKYDOME_STACKS = 32;
const uint SKYDOME_VERTEX_COUNT = SKYDOME_SLICES * (SKYDOME_STACKS + 1);
const uint SKYDOME_TRIANGLE_COUNT = SKYDOME_SLICES * SKYDOME_STACKS * 2;
const uint SKYDOME_INDEX_COUNT = SKYDOME_TRIANGLE_COUNT * 3;
const float SKYDOME_RADIUS = 10.0f;
const float HEIGHT_FACTOR_EXP = 0.7f;
const float STARS_TEX_SIZE = 2.0f;
const float SKY_TEX_STRETCH_FACTOR = 0.4f;

struct SkydomeVertex
{
	Vec3 pos;
	Vec2 tex;
	float latitude;
};

struct VertexShaderDomeGlobals
{
	Matrix mat_combined;
};

struct VertexShaderCloudGlobals
{
	Matrix mat_combined_clouds;
	Vec4 noiseTexA01;
	Vec4 noiseTexA23;
	Vec4 noiseTexB01;
	Vec4 noiseTexB23;
};

struct PixelShaderDomeGlobals
{
	Vec4 horizon_color;
	Vec4 zenith_color;
	float stars_visibility;
	Vec3 _pad;
};

struct PixelShaderCloudGlobals
{
	Vec4 color1;
	Vec4 color2;
	float sharpness;
	float threshold;
	Vec2 _pad;
};

SkyShader::SkyShader(Render* render) : render(render), device_context(nullptr), vertex_shader_dome(nullptr), vertex_shader_celestial(nullptr),
vertex_shader_clouds(nullptr), pixel_shader_dome(nullptr), pixel_shader_celestial(nullptr), pixel_shader_clouds(nullptr), layout_dome(nullptr),
layout_celestial(nullptr), layout_clouds(nullptr), vertex_buffer(nullptr), index_buffer(nullptr), vs_buffer_dome(nullptr), vs_buffer_clouds(nullptr),
ps_buffer_dome(nullptr), ps_buffer_clouds(nullptr), sampler(nullptr)
{
}

SkyShader::~SkyShader()
{
	SafeRelease(vertex_shader_dome);
	SafeRelease(vertex_shader_celestial);
	SafeRelease(vertex_shader_clouds);
	SafeRelease(pixel_shader_dome);
	SafeRelease(pixel_shader_celestial);
	SafeRelease(pixel_shader_clouds);
	SafeRelease(layout_dome);
	SafeRelease(layout_celestial);
	SafeRelease(layout_clouds);
	SafeRelease(vertex_buffer);
	SafeRelease(index_buffer);
	SafeRelease(vs_buffer_dome);
	SafeRelease(vs_buffer_clouds);
	SafeRelease(ps_buffer_dome);
	SafeRelease(ps_buffer_clouds);
	SafeRelease(sampler);
}

void SkyShader::Init()
{
	device_context = render->GetDeviceContext();

	try
	{
		InitInternal();
		CreateMesh();
	}
	catch(cstring err)
	{
		throw Format("Failed to initialize sky shader: %s", err);
	}
}

void SkyShader::InitInternal()
{
	ID3D11Device* device = render->GetDevice();
	HRESULT result;

	// compile shader to blobs
	ID3DBlob* blob_vs_dome = render->CompileShader("sky.hlsl", "skydome_vs", true);
	ID3DBlob* blob_ps_dome = render->CompileShader("sky.hlsl", "skydome_ps", false);
	ID3DBlob* blob_vs_celestial = render->CompileShader("sky.hlsl", "celestial_vs", true);
	ID3DBlob* blob_ps_celestial = render->CompileShader("sky.hlsl", "celestial_ps", false);
	ID3DBlob* blob_vs_clouds = render->CompileShader("sky.hlsl", "clouds_vs", true);
	ID3DBlob* blob_ps_clouds = render->CompileShader("sky.hlsl", "clouds_ps", false);

	// create shaders
	result = device->CreateVertexShader(blob_vs_dome->GetBufferPointer(), blob_vs_dome->GetBufferSize(), nullptr, &vertex_shader_dome);
	if(FAILED(result))
		throw Format("Failed to create dome vertex shader (%u).", result);

	result = device->CreatePixelShader(blob_ps_dome->GetBufferPointer(), blob_ps_dome->GetBufferSize(), nullptr, &pixel_shader_dome);
	if(FAILED(result))
		throw Format("Failed to create dome pixel shader (%u).", result);

	result = device->CreateVertexShader(blob_vs_celestial->GetBufferPointer(), blob_vs_celestial->GetBufferSize(), nullptr, &vertex_shader_celestial);
	if(FAILED(result))
		throw Format("Failed to create celestial vertex shader (%u).", result);

	result = device->CreatePixelShader(blob_ps_celestial->GetBufferPointer(), blob_ps_celestial->GetBufferSize(), nullptr, &pixel_shader_celestial);
	if(FAILED(result))
		throw Format("Failed to create celestial pixel shader (%u).", result);

	result = device->CreateVertexShader(blob_vs_clouds->GetBufferPointer(), blob_vs_clouds->GetBufferSize(), nullptr, &vertex_shader_clouds);
	if(FAILED(result))
		throw Format("Failed to create clouds vertex shader (%u).", result);

	result = device->CreatePixelShader(blob_ps_clouds->GetBufferPointer(), blob_ps_clouds->GetBufferSize(), nullptr, &pixel_shader_clouds);
	if(FAILED(result))
		throw Format("Failed to create clouds pixel shader (%u).", result);

	// create input layouts
	D3D11_INPUT_ELEMENT_DESC skydome_desc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 1, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	D3D11_INPUT_ELEMENT_DESC celestial_desc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	result = device->CreateInputLayout(skydome_desc, countof(skydome_desc), blob_vs_dome->GetBufferPointer(), blob_vs_dome->GetBufferSize(), &layout_dome);
	if(FAILED(result))
		throw Format("Failed to create dome mesh layout (%u).", result);

	result = device->CreateInputLayout(celestial_desc, countof(celestial_desc),
		blob_vs_celestial->GetBufferPointer(), blob_vs_celestial->GetBufferSize(), &layout_celestial);
	if(FAILED(result))
		throw Format("Failed to create celestial mesh layout (%u).", result);

	result = device->CreateInputLayout(skydome_desc, countof(skydome_desc),
		blob_vs_clouds->GetBufferPointer(), blob_vs_clouds->GetBufferSize(), &layout_clouds);
	if(FAILED(result))
		throw Format("Failed to create clouds mesh layout (%u).", result);

	blob_vs_dome->Release();
	blob_ps_dome->Release();
	blob_vs_celestial->Release();
	blob_ps_celestial->Release();
	blob_vs_clouds->Release();
	blob_ps_clouds->Release();

	// create constant buffers
	vs_buffer_dome = render->CreateConstantBuffer(sizeof(VertexShaderDomeGlobals));
	vs_buffer_clouds = render->CreateConstantBuffer(sizeof(VertexShaderCloudGlobals));
	ps_buffer_dome = render->CreateConstantBuffer(sizeof(PixelShaderDomeGlobals));
	ps_buffer_clouds = render->CreateConstantBuffer(sizeof(PixelShaderCloudGlobals));

	// create texture sampler
	D3D11_SAMPLER_DESC sampler_desc;
	sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.MipLODBias = 0.0f;
	sampler_desc.MaxAnisotropy = 1;
	sampler_desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	sampler_desc.BorderColor[0] = 0;
	sampler_desc.BorderColor[1] = 0;
	sampler_desc.BorderColor[2] = 0;
	sampler_desc.BorderColor[3] = 0;
	sampler_desc.MinLOD = 0;
	sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;

	result = device->CreateSamplerState(&sampler_desc, &sampler);
	if(FAILED(result))
		throw Format("Failed to create sampler state (%u).", result);
}

void SkyShader::CreateMesh()
{
	// fill vertices
	vector<SkydomeVertex> vertices;
	vertices.resize(SKYDOME_VERTEX_COUNT);
	SkydomeVertex* v = vertices.data();

	uint latitude, longitude;
	float latitude_f, longitude_f;
	const float latitude_step = PI / (float)SKYDOME_STACKS;
	const float longitude_step = PI * 2 / (float)SKYDOME_SLICES;
	for(latitude = 0, latitude_f = -PI / 2; latitude <= SKYDOME_STACKS; latitude++, latitude_f += latitude_step)
	{
		for(longitude = 0, longitude_f = 0.0f; longitude < SKYDOME_SLICES; longitude++, longitude_f += longitude_step)
		{
			math::SphericalToCartesian(v->pos, longitude_f, latitude_f, SKYDOME_RADIUS);
			float latitude_factor = Max(0.0f, v->pos.y) / SKYDOME_RADIUS;
			latitude_factor = powf(latitude_factor, HEIGHT_FACTOR_EXP);
			v->latitude = latitude_factor;
			float tex_factor = latitude_factor * (1.0f - SKY_TEX_STRETCH_FACTOR) + SKY_TEX_STRETCH_FACTOR;
			v->tex = Vec2(v->pos.x, v->pos.z) / SKYDOME_RADIUS * STARS_TEX_SIZE / tex_factor;
			v++;
		}
	}

	// create vertex buffer
	D3D11_BUFFER_DESC desc;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.ByteWidth = SKYDOME_VERTEX_COUNT * sizeof(SkydomeVertex);
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA subresource;
	subresource.pSysMem = vertices.data();

	HRESULT result = render->GetDevice()->CreateBuffer(&desc, &subresource, &vertex_buffer);
	if(FAILED(result))
		throw Format("Failed to create vertex buffer (%u).", result);

	// fill indices
	vector<word> indices;
	indices.resize(SKYDOME_INDEX_COUNT);
	word* index = indices.data();

	for(uint latitude = 0; latitude < SKYDOME_STACKS; latitude++)
	{
		for(uint longitude = 0; longitude < SKYDOME_SLICES; longitude++)
		{
			*index = (latitude)* SKYDOME_SLICES + (longitude + 1) % SKYDOME_SLICES; index++;
			*index = (latitude + 1) * SKYDOME_SLICES + (longitude + 1) % SKYDOME_SLICES; index++;
			*index = (latitude + 1) * SKYDOME_SLICES + (longitude); index++;
			*index = (latitude)* SKYDOME_SLICES + (longitude + 1) % SKYDOME_SLICES; index++;
			*index = (latitude + 1) * SKYDOME_SLICES + (longitude); index++;
			*index = (latitude)* SKYDOME_SLICES + (longitude); index++;
		}
	}

	// create index buffer
	desc.ByteWidth = SKYDOME_INDEX_COUNT * sizeof(word);
	desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	subresource.pSysMem = indices.data();

	result = render->GetDevice()->CreateBuffer(&desc, &subresource, &index_buffer);
	if(FAILED(result))
		throw Format("Failed to create index buffer (%u).", result);
}

void SkyShader::Draw(Sky* sky, const Matrix& mat_combined)
{
	assert(sky);
	this->sky = sky;

	device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// TODO: move common things here
	DrawSkydome(mat_combined);
	// celestials require WARP tex?
	DrawClouds(mat_combined);

	/*params.SetAlphaBlendEnable(true);
	params.SetBlendOp(D3DBLENDOP_ADD);

	DrawCelestialObjects(camera, mat);

	if(clouds.threshold < 0.99f)
		DrawClouds(camera, mat);*/
}

void SkyShader::DrawSkydome(const Matrix& mat_combined)
{
	render->SetCulling(false);
	render->SetDepthState(Render::DEPTH_NO);

	device_context->IASetInputLayout(layout_dome);
	device_context->VSSetShader(vertex_shader_dome, nullptr, 0);
	device_context->VSSetConstantBuffers(0, 1, &vs_buffer_dome);
	device_context->PSSetShader(pixel_shader_dome, nullptr, 0);
	device_context->PSSetConstantBuffers(0, 1, &ps_buffer_dome);
	device_context->PSSetSamplers(0, 1, &sampler);

	D3D11_MAPPED_SUBRESOURCE resource;
	C(device_context->Map(vs_buffer_dome, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
	VertexShaderDomeGlobals& vs_g = *(VertexShaderDomeGlobals*)resource.pData;
	vs_g.mat_combined = mat_combined.Transpose();
	device_context->Unmap(vs_buffer_dome, 0);

	C(device_context->Map(ps_buffer_dome, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
	PixelShaderDomeGlobals& ps_g = *(PixelShaderDomeGlobals*)resource.pData;
	ps_g.horizon_color = Vec4(1, 0, 0, 1);
	ps_g.zenith_color = Vec4(0, 1, 0, 1);
	ps_g.stars_visibility = 0.f;
	device_context->Unmap(ps_buffer_dome, 0);
	// TODO: values, texture

	uint stride = sizeof(SkydomeVertex),
		offset = 0;
	device_context->IASetVertexBuffers(0, 1, &vertex_buffer, &stride, &offset);
	device_context->IASetIndexBuffer(index_buffer, DXGI_FORMAT_R16_UINT, 0);
	device_context->DrawIndexed(SKYDOME_INDEX_COUNT, 0, 0);
}

/*if(!(sun.enabled || moon.enabled))
	{
		// nothing to draw
		return;
	}

	FrameParams& params = Render.GetFrameParams();
	uint passes;

	params.SetFVF(SimpleVertex::fvf);
	params.SetSrcBlend(D3DBLEND_SRCALPHA);
	params.SetDestBlend(D3DBLEND_INVSRCALPHA);

	params.SetTextureStageColorOp(0, D3DTOP_MODULATE);
	params.SetTextureStageColorArg1(0, D3DTA_TEXTURE);
	params.SetTextureStageColorArg2(0, D3DTA_TFACTOR);
	params.SetTextureStageAlphaOp(0, D3DTOP_MODULATE);
	params.SetTextureStageAlphaArg1(0, D3DTA_TEXTURE);
	params.SetTextureStageAlphaArg2(0, D3DTA_TFACTOR);

	V( e->SetTechnique(e->GetTechniqueByName("Celestial")) );
	V( e->SetMatrix(hMatCombined, &matCombined) );
	
	V( e->Begin(&passes, D3DFX_DONTSAVE) );
	V( e->BeginPass(0) );

	// sun
	if(sun.enabled && sun.tex)
	{
		params.SetTextureFactor(sun.color_horizon.ToUint());
		V( e->SetTexture(hTex, sun.tex->tex) );
		V( e->CommitChanges() );

		sun.EnsureVertices();
		V( params.device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, sun.v, sizeof(SimpleVertex)) );
	}

	// moon
	if(moon.enabled && moon.tex)
	{
		params.SetTextureFactor(moon.color_horizon.ToUint());
		V( e->SetTexture(hTex, moon.tex->tex) );
		V( e->CommitChanges() );

		moon.EnsureVertices();
		V( params.device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, moon.v, sizeof(SimpleVertex)) );
	}

	// sun glow
if(sun.enabled && tSunGlow)
{
	params.SetDestBlend(D3DBLEND_ONE);
	sun.EnsureGlowVertices();

	float intensity = powf(sun.dir.y, HEIGHT_FACTOR_EXP) * sun.glow;

	params.SetTextureFactor(0x00FFFFFF | ((uint)(intensity * 255.0f) << 24));
	V(e->SetTexture(hTex, tSunGlow->tex));
	V(e->CommitChanges());

	V(params.device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, sun.gv, sizeof(SimpleVertex)));
}

V(e->EndPass());
V(e->End());

// restore default texture stage params
params.RestoreTextureStage(0); */

void SkyShader::DrawClouds(const Matrix& mat_combined)
{
	Vec2 noiseTexA[4];
	Vec2 noiseTexB[4];
	Vec2 octave_movement = sky->clouds_offset;
	for(uint i = 0; i<4; ++i)
	{
		noiseTexA[i].x = noiseTexA[i].y = 1.f / expf(float(3 - i)) * sky->clouds_scale_inv;
		noiseTexB[i] = octave_movement;
		noiseTexB[i].x *= noiseTexA[i].x;
		noiseTexB[i].y *= noiseTexA[i].y;
		octave_movement *= 0.7f;
	}

	device_context->IASetInputLayout(layout_clouds);
	device_context->VSSetShader(vertex_shader_clouds, nullptr, 0);
	device_context->VSSetConstantBuffers(0, 1, &vs_buffer_clouds);
	device_context->PSSetShader(pixel_shader_clouds, nullptr, 0);
	device_context->PSSetConstantBuffers(0, 1, &ps_buffer_clouds);
	device_context->PSSetSamplers(0, 1, &sampler);
	ID3D11ShaderResourceView* tex[1] = { sky->tex_clouds_noise ? sky->tex_clouds_noise->tex : nullptr };
	device_context->PSSetShaderResources(0, 1, tex);

	//Color4f_Pair colors;
	//clouds.colors.Get(&colors, math::Frac(time));
	//colors.first *= clouds.color_weather_factors.first;
	//colors.second *= clouds.color_weather_factors.second;

	//params.SetSrcBlend(D3DBLEND_SRCALPHA);
	//params.SetDestBlend(D3DBLEND_INVSRCALPHA);

	D3D11_MAPPED_SUBRESOURCE resource;
	C(device_context->Map(vs_buffer_clouds, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
	VertexShaderCloudGlobals& vs_g = *(VertexShaderCloudGlobals*)resource.pData;
	vs_g.mat_combined_clouds = mat_combined.Transpose();
	vs_g.noiseTexA01 = Vec4(noiseTexA[0].x, noiseTexA[0].y, noiseTexA[1].x, noiseTexA[1].y);
	vs_g.noiseTexA23 = Vec4(noiseTexA[2].x, noiseTexA[2].y, noiseTexA[3].x, noiseTexA[3].y);
	vs_g.noiseTexB01 = Vec4(noiseTexB[0].x, noiseTexB[0].y, noiseTexB[1].x, noiseTexB[1].y);
	vs_g.noiseTexB23 = Vec4(noiseTexB[2].x, noiseTexB[2].y, noiseTexB[3].x, noiseTexB[3].y);
	device_context->Unmap(vs_buffer_clouds, 0);

	C(device_context->Map(ps_buffer_clouds, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
	PixelShaderCloudGlobals& ps_g = *(PixelShaderCloudGlobals*)resource.pData;
	ps_g.color1 = sky->clouds_color[0];
	ps_g.color2 = sky->clouds_color[1];
	ps_g.sharpness = sky->clouds_sharpness;
	ps_g.threshold = sky->clouds_threshold;
	device_context->Unmap(ps_buffer_clouds, 0);

	uint stride = sizeof(SkydomeVertex),
		offset = 0;
	device_context->IASetVertexBuffers(0, 1, &vertex_buffer, &stride, &offset);
	device_context->IASetIndexBuffer(index_buffer, DXGI_FORMAT_R16_UINT, 0);
	device_context->DrawIndexed(SKYDOME_INDEX_COUNT, 0, 0);
}
