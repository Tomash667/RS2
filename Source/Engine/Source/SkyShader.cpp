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

struct PixelShaderCelestialGlobals
{
	Vec4 celestial_color;
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
layout_celestial(nullptr), layout_clouds(nullptr), vertex_buffer(nullptr), vertex_buffer_celestial(nullptr), index_buffer(nullptr),
index_buffer_celestial(nullptr), vs_buffer_dome(nullptr), vs_buffer_clouds(nullptr), ps_buffer_dome(nullptr), ps_buffer_celestial(nullptr),
ps_buffer_clouds(nullptr), sampler(nullptr)
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
	SafeRelease(vertex_buffer_celestial);
	SafeRelease(index_buffer);
	SafeRelease(index_buffer_celestial);
	SafeRelease(vs_buffer_dome);
	SafeRelease(vs_buffer_clouds);
	SafeRelease(ps_buffer_dome);
	SafeRelease(ps_buffer_celestial);
	SafeRelease(ps_buffer_clouds);
	SafeRelease(sampler);
}

void SkyShader::Init()
{
	device_context = render->GetDeviceContext();

	try
	{
		InitInternal();
		CreateDomeMesh();
		CreateCelestialMesh();
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
	ps_buffer_celestial = render->CreateConstantBuffer(sizeof(PixelShaderCelestialGlobals));
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

void SkyShader::CreateDomeMesh()
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
			math::SphericalToCartesian(v->pos, longitude_f, latitude_f, Sky::SKYDOME_RADIUS);
			float latitude_factor = Max(0.0f, v->pos.y) / Sky::SKYDOME_RADIUS;
			latitude_factor = powf(latitude_factor, HEIGHT_FACTOR_EXP);
			v->latitude = latitude_factor;
			float tex_factor = latitude_factor * (1.0f - SKY_TEX_STRETCH_FACTOR) + SKY_TEX_STRETCH_FACTOR;
			v->tex = Vec2(v->pos.x, v->pos.z) / Sky::SKYDOME_RADIUS * STARS_TEX_SIZE / tex_factor;
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

void SkyShader::CreateCelestialMesh()
{
	// create vertex buffer
	D3D11_BUFFER_DESC desc;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.ByteWidth = 4 * sizeof(SimpleVertex);
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;

	HRESULT result = render->GetDevice()->CreateBuffer(&desc, nullptr, &vertex_buffer_celestial);
	if(FAILED(result))
		throw Format("Failed to create celestial vertex buffer (%u).", result);

	// create index buffer
	desc.ByteWidth = 6 * sizeof(word);
	desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.CPUAccessFlags = 0;

	word indices[] = { 0, 1, 2, 2, 1, 3 };
	D3D11_SUBRESOURCE_DATA subresource;
	subresource.pSysMem = indices;

	result = render->GetDevice()->CreateBuffer(&desc, &subresource, &index_buffer_celestial);
	if(FAILED(result))
		throw Format("Failed to create celestial index buffer (%u).", result);
}

void SkyShader::Draw(Sky* sky, const Matrix& mat_combined)
{
	assert(sky);
	this->sky = sky;

	DrawSkydome(mat_combined);
	DrawCelestialObjects();
	if(sky->clouds_threshold < 1.2f)
		DrawClouds(mat_combined);
}

void SkyShader::DrawSkydome(const Matrix& mat_combined)
{
	render->SetCulling(false);
	render->SetAlphaBlend(Render::BLEND_NO);
	render->SetDepthState(Render::DEPTH_NO);

	uint stride = sizeof(SkydomeVertex),
		offset = 0;

	device_context->IASetInputLayout(layout_dome);
	device_context->IASetVertexBuffers(0, 1, &vertex_buffer, &stride, &offset);
	device_context->IASetIndexBuffer(index_buffer, DXGI_FORMAT_R16_UINT, 0);
	device_context->VSSetShader(vertex_shader_dome, nullptr, 0);
	device_context->VSSetConstantBuffers(0, 1, &vs_buffer_dome);
	device_context->PSSetShader(pixel_shader_dome, nullptr, 0);
	device_context->PSSetConstantBuffers(0, 1, &ps_buffer_dome);
	device_context->PSSetSamplers(0, 1, &sampler);
	ID3D11ShaderResourceView* tex[1] = { sky->tex_stars ? sky->tex_stars->tex : nullptr };
	device_context->PSSetShaderResources(0, 1, tex);

	D3D11_MAPPED_SUBRESOURCE resource;
	C(device_context->Map(vs_buffer_dome, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
	VertexShaderDomeGlobals& vs_g = *(VertexShaderDomeGlobals*)resource.pData;
	vs_g.mat_combined = mat_combined.Transpose();
	device_context->Unmap(vs_buffer_dome, 0);

	C(device_context->Map(ps_buffer_dome, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
	PixelShaderDomeGlobals& ps_g = *(PixelShaderDomeGlobals*)resource.pData;
	ps_g.horizon_color = sky->horizon_color;
	ps_g.zenith_color = sky->zenith_color;
	ps_g.stars_visibility = sky->stars_visibility;
	device_context->Unmap(ps_buffer_dome, 0);

	device_context->DrawIndexed(SKYDOME_INDEX_COUNT, 0, 0);
}

void SkyShader::DrawCelestialObjects()
{
	if(!sky->sun.enabled && !sky->moon.enabled)
		return;

	ID3D11ShaderResourceView* tex[1];
	SimpleVertex v[4];
	uint stride = sizeof(SimpleVertex),
		offset = 0;

	// uses same vs globals, don't set here
	device_context->IASetInputLayout(layout_celestial);
	device_context->IASetVertexBuffers(0, 1, &vertex_buffer_celestial, &stride, &offset);
	device_context->IASetIndexBuffer(index_buffer_celestial, DXGI_FORMAT_R16_UINT, 0);
	device_context->VSSetShader(vertex_shader_celestial, nullptr, 0);
	device_context->PSSetShader(pixel_shader_celestial, nullptr, 0);
	device_context->PSSetConstantBuffers(0, 1, &ps_buffer_celestial);
	device_context->PSSetSamplers(0, 1, &sampler);

	render->SetAlphaBlend(Render::BLEND_NORMAL);

	// draw sun
	if(sky->sun.enabled)
	{
		tex[0] = sky->sun.texture ? sky->sun.texture->tex : nullptr;
		device_context->PSSetShaderResources(0, 1, tex);

		D3D11_MAPPED_SUBRESOURCE resource;
		C(device_context->Map(ps_buffer_celestial, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
		PixelShaderCelestialGlobals& ps_g = *(PixelShaderCelestialGlobals*)resource.pData;
		ps_g.celestial_color = sky->sun.color;
		device_context->Unmap(ps_buffer_celestial, 0);

		sky->sun.GetVertices(v);
		D3D11_MAPPED_SUBRESOURCE subresource;
		C(device_context->Map(vertex_buffer_celestial, 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource));
		memcpy(subresource.pData, v, sizeof(v));
		device_context->Unmap(vertex_buffer_celestial, 0);

		device_context->DrawIndexed(6, 0, 0);
	}

	// draw moon
	if(sky->moon.enabled)
	{
		tex[0] = sky->moon.texture ? sky->moon.texture->tex : nullptr;
		device_context->PSSetShaderResources(0, 1, tex);

		D3D11_MAPPED_SUBRESOURCE resource;
		C(device_context->Map(ps_buffer_celestial, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
		PixelShaderCelestialGlobals& ps_g = *(PixelShaderCelestialGlobals*)resource.pData;
		ps_g.celestial_color = sky->moon.color;
		device_context->Unmap(ps_buffer_celestial, 0);

		sky->moon.GetVertices(v);
		D3D11_MAPPED_SUBRESOURCE subresource;
		C(device_context->Map(vertex_buffer_celestial, 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource));
		memcpy(subresource.pData, v, sizeof(v));
		device_context->Unmap(vertex_buffer_celestial, 0);

		device_context->DrawIndexed(6, 0, 0);
	}
}

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

	render->SetAlphaBlend(Render::BLEND_NORMAL);

	device_context->IASetInputLayout(layout_clouds);
	device_context->VSSetShader(vertex_shader_clouds, nullptr, 0);
	device_context->VSSetConstantBuffers(0, 1, &vs_buffer_clouds);
	device_context->PSSetShader(pixel_shader_clouds, nullptr, 0);
	device_context->PSSetConstantBuffers(0, 1, &ps_buffer_clouds);
	device_context->PSSetSamplers(0, 1, &sampler);
	ID3D11ShaderResourceView* tex[1] = { sky->tex_clouds_noise ? sky->tex_clouds_noise->tex : nullptr };
	device_context->PSSetShaderResources(0, 1, tex);

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
