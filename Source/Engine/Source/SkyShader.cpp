#include "EngineCore.h"
#include "SkyShader.h"
#include "Render.h"
#include "Mesh.h"
#include "Vertex.h"
#include "Texture.h"
#include <d3d11_1.h>
#include "InternalHelper.h"

SkyShader::SkyShader(Render* render) : render(render), device_context(nullptr), sampler(nullptr)
{
}

SkyShader::~SkyShader()
{
	SafeRelease(sampler);
}

void SkyShader::Init()
{
	device_context = render->GetDeviceContext();

	// create shader
	D3D11_INPUT_ELEMENT_DESC dome_desc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 1, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	D3D11_INPUT_ELEMENT_DESC obj_desc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	uint cbuffer_size[] = { sizeof(Matrix), 0 };
	render->CreateShader(shader, "skybox.hlsl", desc, countof(desc), cbuffer_size);
	shader.vertex_stride = sizeof(Vertex);

	// create texture sampler
	D3D11_SAMPLER_DESC sampler_desc;
	sampler_desc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
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
		throw Format("Failed to create skybox sampler state (%u).", result);
}

void SkyShader::Draw(Mesh* mesh, const Vec3& center, const Matrix& mat_view_proj)
{
	render->SetAlphaBlend(false);
	render->SetDepthState(Render::DEPTH_READONLY);
	render->SetCulling(true);

	device_context->IASetInputLayout(shader.layout);
	device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	device_context->VSSetShader(shader.vertex_shader, nullptr, 0);
	device_context->VSSetConstantBuffers(0, 1, &shader.vs_buffer);
	device_context->PSSetShader(shader.pixel_shader, nullptr, 0);
	device_context->PSSetSamplers(0, 1, &sampler);

	Matrix m = Matrix::Translation(center) * mat_view_proj;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	C(device_context->Map(shader.vs_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));
	Matrix& g = *(Matrix*)mappedResource.pData;
	g = m.Transpose();
	device_context->Unmap(shader.vs_buffer, 0);

	uint stride = sizeof(Vertex),
		offset = 0;
	device_context->IASetVertexBuffers(0, 1, &mesh->vb, &stride, &offset);
	device_context->IASetIndexBuffer(mesh->ib, DXGI_FORMAT_R16_UINT, 0);

	for(Mesh::Submesh& sub : mesh->subs)
	{
		device_context->PSSetShaderResources(0, 1, &sub.tex->tex);
		device_context->DrawIndexed(sub.tris * 3, sub.first * 3, sub.min_ind);
	}
}
