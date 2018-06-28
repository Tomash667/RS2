#include "EngineCore.h"
#include "ParticleShader.h"
#include "Render.h"
#include "ParticleEmitter.h"
#include "Texture.h"
#include "Vertex.h"
#include <d3d11_1.h>
#include "InternalHelper.h"

ParticleShader::ParticleShader(Render* render) : render(render), device_context(nullptr), sampler(nullptr), vb(nullptr), vb_size(0)
{
}

ParticleShader::~ParticleShader()
{
	SafeRelease(vb);
	SafeRelease(sampler);
}

void ParticleShader::Init()
{
	device_context = render->GetDeviceContext();

	// compile shader
	D3D11_INPUT_ELEMENT_DESC desc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	uint cbuffer_size[] = { sizeof(Matrix), 0 };
	render->CreateShader(shader, "particle.hlsl", desc, countof(desc), cbuffer_size);
	shader.vertex_stride = sizeof(ParticleVertex);

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

	HRESULT result = render->GetDevice()->CreateSamplerState(&sampler_desc, &sampler);
	if(FAILED(result))
		throw Format("Failed to create sampler state (%u).", result);
}

void ParticleShader::Prepare(const Matrix& mat_view, const Matrix& mat_view_proj)
{
	mat_view_inv = mat_view.Inverse();

	render->SetAlphaBlend(Render::BLEND_NORMAL);
	render->SetDepthState(Render::DEPTH_READONLY);
	render->SetCulling(false);

	device_context->IASetInputLayout(shader.layout);
	device_context->VSSetShader(shader.vertex_shader, nullptr, 0);
	device_context->VSSetConstantBuffers(0, 1, &shader.vs_buffer);
	device_context->PSSetShader(shader.pixel_shader, nullptr, 0);
	device_context->PSSetSamplers(0, 1, &sampler);

	// set vertex shader constants
	D3D11_MAPPED_SUBRESOURCE resource;
	C(device_context->Map(shader.vs_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
	Matrix& m = *(Matrix*)resource.pData;
	m = mat_view_proj.Transpose();
	device_context->Unmap(shader.vs_buffer, 0);

	// set vb
	if(vb)
	{
		uint stride = shader.vertex_stride,
			offset = 0;
		device_context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
	}
}

void ParticleShader::Draw(ParticleEmitter* pe)
{
	assert(pe);

	// set texture
	device_context->PSSetShaderResources(0, 1, &pe->tex->tex);

	// resize vb if required
	uint vertex_count = pe->alive * 6;
	if(vertex_count > vb_size)
	{
		SafeRelease(vb);
		vb_size = vertex_count;

		D3D11_BUFFER_DESC v_desc;
		v_desc.Usage = D3D11_USAGE_DYNAMIC;
		v_desc.ByteWidth = vb_size * sizeof(ParticleVertex);
		v_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		v_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		v_desc.MiscFlags = 0;
		v_desc.StructureByteStride = 0;

		HRESULT result = render->GetDevice()->CreateBuffer(&v_desc, nullptr, &vb);
		if(FAILED(result))
			throw Format("Failed to create particle vertex buffer (%u).", result);

		uint stride = shader.vertex_stride,
			offset = 0;
		device_context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
	}

	// fill vb
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	C(device_context->Map(vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));
	ParticleVertex* v = (ParticleVertex*)mappedResource.pData;
	int index = 0;
	for(ParticleEmitter::Particle& p : pe->particles)
	{
		if(!p.exists)
			continue;

		mat_view_inv._41 = p.pos.x;
		mat_view_inv._42 = p.pos.y;
		mat_view_inv._43 = p.pos.z;
		Matrix m = Matrix::Scale(pe->GetScale(p)) * mat_view_inv;

		const Vec4 color(1.f, 1.f, 1.f, pe->GetAlpha(p));

		v[index].pos = Vec3::Transform(Vec3(-1, -1, 0), m);
		v[index].tex = Vec2(0, 0);
		v[index].color = color;

		v[index + 1].pos = Vec3::Transform(Vec3(-1, 1, 0), m);
		v[index + 1].tex = Vec2(0, 1);
		v[index + 1].color = color;

		v[index + 2].pos = Vec3::Transform(Vec3(1, -1, 0), m);
		v[index + 2].tex = Vec2(1, 0);
		v[index + 2].color = color;

		v[index + 3] = v[index + 1];

		v[index + 4].pos = Vec3::Transform(Vec3(1, 1, 0), m);
		v[index + 4].tex = Vec2(1, 1);
		v[index + 4].color = color;

		v[index + 5] = v[index + 2];

		index += 6;
	}
	device_context->Unmap(vb, 0);

	// draw
	device_context->Draw(pe->alive * 6, 0);
}
