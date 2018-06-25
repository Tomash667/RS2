#include "EngineCore.h"
#include "SkyShader.h"
#include "Render.h"
#include "Mesh.h"
#include "Vertex.h"
#include "Texture.h"
#include <d3d11_1.h>
#include "InternalHelper.h"

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
layout_celestial(nullptr), layout_clouds(nullptr), vs_buffer_dome(nullptr), vs_buffer_clouds(nullptr), ps_buffer_dome(nullptr), ps_buffer_clouds(nullptr),
sampler(nullptr)
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

void SkyShader::Draw()
{
}
