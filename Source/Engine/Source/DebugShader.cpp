#include "EngineCore.h"
#include "DebugShader.h"
#include "Render.h"
#include "Mesh.h"
#include <d3d11_1.h>
#include "InternalHelper.h"

DebugShader::DebugShader(Render* render) : render(render), device_context(nullptr), vb(nullptr), locked(false)
{
}

DebugShader::~DebugShader()
{
	SafeRelease(vb);
}

void DebugShader::Init()
{
	device_context = render->GetDeviceContext();

	// compile shader
	D3D11_INPUT_ELEMENT_DESC desc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	uint cbuffer_size[] = { sizeof(Matrix), sizeof(Vec4) };
	render->CreateShader(shader, "debug.hlsl", desc, countof(desc), cbuffer_size);
	shader.vertex_stride = sizeof(Vec3);

	// create vertex buffer
	D3D11_BUFFER_DESC v_desc;
	v_desc.Usage = D3D11_USAGE_DYNAMIC;
	v_desc.ByteWidth = MAX_VERTICES * sizeof(Vec3);
	v_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	v_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	v_desc.MiscFlags = 0;
	v_desc.StructureByteStride = 0;

	HRESULT result = render->GetDevice()->CreateBuffer(&v_desc, nullptr, &vb);
	if(FAILED(result))
		throw Format("Failed to create debug vertex buffer (%u).", result);
}

void DebugShader::Prepare(const Matrix& mat_view_proj)
{
	assert(!locked);

	this->mat_view_proj = mat_view_proj;

	render->SetAlphaBlend(Render::BLEND_NORMAL);
	render->SetDepthState(Render::DEPTH_READONLY);
	render->SetCulling(false);

	device_context->IASetInputLayout(shader.layout);
	device_context->VSSetShader(shader.vertex_shader, nullptr, 0);
	device_context->VSSetConstantBuffers(0, 1, &shader.vs_buffer);
	device_context->PSSetShader(shader.pixel_shader, nullptr, 0);
	device_context->PSSetConstantBuffers(0, 1, &shader.ps_buffer);

	current_vb = nullptr;
}

void DebugShader::SetColor(Color color)
{
	D3D11_MAPPED_SUBRESOURCE resource;
	C(device_context->Map(shader.ps_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
	Vec4& c = *(Vec4*)resource.pData;
	c = color;
	device_context->Unmap(shader.ps_buffer, 0);
}

Vec3* DebugShader::Lock()
{
	assert(!locked);

	if(current_vb != vb)
	{
		// set vertex shader constants
		D3D11_MAPPED_SUBRESOURCE resource;
		C(device_context->Map(shader.vs_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
		Matrix& m = *(Matrix*)resource.pData;
		m = mat_view_proj.Transpose();
		device_context->Unmap(shader.vs_buffer, 0);

		// set vb
		uint stride = shader.vertex_stride,
			offset = 0;
		device_context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);

		current_vb = vb;
	}

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	C(device_context->Map(vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));
	locked = true;
	return (Vec3*)mappedResource.pData;
}

void DebugShader::Draw(uint vertex_count)
{
	assert(locked && vertex_count > 0 && vertex_count <= MAX_VERTICES);
	locked = false;
	device_context->Unmap(vb, 0);
	device_context->Draw(vertex_count, 0);
}

void DebugShader::Draw(Mesh* mesh, const Matrix& mat_world)
{
	assert(!locked);

	if(current_vb != mesh->vb)
	{
		// set vb
		uint stride = shader.vertex_stride,
			offset = 0;
		device_context->IASetVertexBuffers(0, 1, &mesh->vb, &stride, &offset);
		device_context->IASetIndexBuffer(mesh->ib, DXGI_FORMAT_R16_UINT, 0);

		current_vb = mesh->vb;
	}

	// set vertex shader constants
	D3D11_MAPPED_SUBRESOURCE resource;
	C(device_context->Map(shader.vs_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
	Matrix& m = *(Matrix*)resource.pData;
	m = (mat_world * mat_view_proj).Transpose();
	device_context->Unmap(shader.vs_buffer, 0);

	// draw submeshes
	for(Mesh::Submesh& sub : mesh->subs)
		device_context->DrawIndexed(sub.tris * 3, sub.first * 3, sub.min_ind);
}
