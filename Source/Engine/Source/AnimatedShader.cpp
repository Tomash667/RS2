#include "EngineCore.h"
#include "AnimatedShader.h"
#include "Render.h"
#include "MeshInstance.h"
#include "Texture.h"
#include "Vertex.h"
#include <d3d11_1.h>
#include "InternalHelper.h"

AnimatedShader::AnimatedShader(Render* render) : render(render), device_context(nullptr), sampler(nullptr)
{
}

AnimatedShader::~AnimatedShader()
{
	SafeRelease(sampler);
}

void AnimatedShader::Init()
{
	device_context = render->GetDeviceContext();

	// compile shader
	D3D11_INPUT_ELEMENT_DESC desc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BLENDWEIGHT", 0, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BLENDINDICES", 0, DXGI_FORMAT_R8G8B8A8_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	uint cbuffer_size[] = { sizeof(VertexShaderGlobals), sizeof(PixelShaderGlobals) };
	render->CreateShader(shader, "animated.hlsl", desc, countof(desc), cbuffer_size);
	shader.vertex_stride = sizeof(AniVertex);

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

void AnimatedShader::SetParams(const Vec4& fog_color, const Vec4& fog_params)
{
	this->fog_color = fog_color;
	this->fog_params = fog_params;
}

void AnimatedShader::Prepare()
{
	device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	device_context->IASetInputLayout(shader.layout);
	device_context->VSSetShader(shader.vertex_shader, nullptr, 0);
	device_context->VSSetConstantBuffers(0, 1, &shader.vs_buffer);
	device_context->PSSetShader(shader.pixel_shader, nullptr, 0);
	device_context->PSSetConstantBuffers(0, 1, &shader.ps_buffer);
	device_context->PSSetSamplers(0, 1, &sampler);
}

void AnimatedShader::DrawMesh(Mesh* mesh, MeshInstance* mesh_inst, const Matrix& mat_combined, const Vec3& tint, int subs)
{
	assert(mesh && mesh_inst);

	// set vertex shader constants
	D3D11_MAPPED_SUBRESOURCE resource;
	C(device_context->Map(shader.vs_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
	VertexShaderGlobals& vsg = *(VertexShaderGlobals*)resource.pData;
	vsg.mat_combined = mat_combined.Transpose();
	mesh_inst->SetupBones();
	const vector<Matrix>& bones = mesh_inst->GetMatrixBones();
	for(size_t i = 0, count = bones.size(); i < count; ++i)
		vsg.mat_bones[i] = bones[i].Transpose();
	device_context->Unmap(shader.vs_buffer, 0);

	// set pixel shader constants
	C(device_context->Map(shader.ps_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
	PixelShaderGlobals& psg = *(PixelShaderGlobals*)resource.pData;
	psg.fog_color = fog_color;
	psg.fog_params = fog_params;
	psg.tint = Vec4(tint, 1.f);
	device_context->Unmap(shader.ps_buffer, 0);

	// set buffers
	uint stride = shader.vertex_stride,
		offset = 0;
	device_context->IASetVertexBuffers(0, 1, &mesh->vb, &stride, &offset);
	device_context->IASetIndexBuffer(mesh->ib, DXGI_FORMAT_R16_UINT, 0);

	// draw submeshes
	int index = 1 << 0;
	for(Mesh::Submesh& sub : mesh->subs)
	{
		if(IS_SET(subs, index))
		{
			assert(sub.tex && sub.tex->tex);
			device_context->PSSetShaderResources(0, 1, &sub.tex->tex);
			device_context->DrawIndexed(sub.tris * 3, sub.first * 3, sub.min_ind);
		}
		index <<= 1;
	}
}
