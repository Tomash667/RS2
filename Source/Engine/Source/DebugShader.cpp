#include "EngineCore.h"
#include "DebugShader.h"
#include "Render.h"
#include "Mesh.h"
#include <d3d11_1.h>
#include "InternalHelper.h"

DebugShader::DebugShader(Render* render) : render(render), device_context(nullptr), vb(nullptr), max_verts(6u), vertex_shader(nullptr),
vertex_shader_color(nullptr), pixel_shader(nullptr), pixel_shader_color(nullptr), layout(nullptr), layout_color(nullptr), vs_buffer(nullptr),
ps_buffer(nullptr)
{
}

DebugShader::~DebugShader()
{
	SafeRelease(vb);
	SafeRelease(vertex_shader);
	SafeRelease(vertex_shader_color);
	SafeRelease(pixel_shader);
	SafeRelease(pixel_shader_color);
	SafeRelease(layout);
	SafeRelease(layout_color);
	SafeRelease(vs_buffer);
	SafeRelease(ps_buffer);
}

void DebugShader::Init()
{
	device_context = render->GetDeviceContext();

	try
	{
		InitInternal();
	}
	catch(cstring err)
	{
		throw Format("Failed to initialize debug shader: %s", err);
	}
}

void DebugShader::InitInternal()
{
	ID3D11Device* device = render->GetDevice();
	HRESULT result;

	// compile shader to blobs
	ID3DBlob* blob_vs = render->CompileShader("debug.hlsl", "vs_main", true);
	ID3DBlob* blob_vs_color = render->CompileShader("debug.hlsl", "vs_main_color", true);
	ID3DBlob* blob_ps = render->CompileShader("debug.hlsl", "ps_main", false);
	ID3DBlob* blob_ps_color = render->CompileShader("debug.hlsl", "ps_main_color", false);

	// create shaders
	result = device->CreateVertexShader(blob_vs->GetBufferPointer(), blob_vs->GetBufferSize(), nullptr, &vertex_shader);
	if(FAILED(result))
		throw Format("Failed to create vertex shader (%u).", result);

	result = device->CreateVertexShader(blob_vs_color->GetBufferPointer(), blob_vs_color->GetBufferSize(), nullptr, &vertex_shader_color);
	if(FAILED(result))
		throw Format("Failed to create color vertex shader (%u).", result);

	result = device->CreatePixelShader(blob_ps->GetBufferPointer(), blob_ps->GetBufferSize(), nullptr, &pixel_shader);
	if(FAILED(result))
		throw Format("Failed to create pixel shader (%u).", result);

	result = device->CreatePixelShader(blob_ps_color->GetBufferPointer(), blob_ps_color->GetBufferSize(), nullptr, &pixel_shader_color);
	if(FAILED(result))
		throw Format("Failed to create color pixel shader (%u).", result);

	// create input layouts
	D3D11_INPUT_ELEMENT_DESC desc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	D3D11_INPUT_ELEMENT_DESC desc_color[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	result = device->CreateInputLayout(desc, countof(desc), blob_vs->GetBufferPointer(), blob_vs->GetBufferSize(), &layout);
	if(FAILED(result))
		throw Format("Failed to create layout (%u).", result);

	result = device->CreateInputLayout(desc_color, countof(desc_color), blob_vs_color->GetBufferPointer(), blob_vs_color->GetBufferSize(),
		&layout_color);
	if(FAILED(result))
		throw Format("Failed to create color layout (%u).", result);

	blob_vs->Release();
	blob_vs_color->Release();
	blob_ps->Release();
	blob_ps_color->Release();

	// create constant buffers
	vs_buffer = render->CreateConstantBuffer(sizeof(Matrix));
	ps_buffer = render->CreateConstantBuffer(sizeof(Vec4));

	CreateVertexBuffer();
}

void DebugShader::CreateVertexBuffer()
{
	SafeRelease(vb);

	D3D11_BUFFER_DESC v_desc;
	v_desc.Usage = D3D11_USAGE_DYNAMIC;
	v_desc.ByteWidth = max_verts * sizeof(ColorVertex);
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
	this->mat_view_proj = mat_view_proj;

	render->SetAlphaBlend(Render::BLEND_NORMAL);
	render->SetDepthState(Render::DEPTH_READONLY);
	render->SetCulling(false);

	device_context->VSSetConstantBuffers(0, 1, &vs_buffer);
	device_context->PSSetConstantBuffers(0, 1, &ps_buffer);

	current_vb = nullptr;
	prev_color = Color(0, 0, 0, 0);
}

void DebugShader::Draw(const vector<ColorVertex>& verts)
{
	if(verts.size() > max_verts)
	{
		max_verts = verts.size();
		CreateVertexBuffer();
	}

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	C(device_context->Map(vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));
	memcpy(mappedResource.pData, verts.data(), sizeof(ColorVertex) * verts.size());
	device_context->Unmap(vb, 0);

	if(vb != current_vb)
	{
		current_vb = vb;
		device_context->IASetInputLayout(layout_color);
		device_context->VSSetShader(vertex_shader_color, nullptr, 0);
		device_context->PSSetShader(pixel_shader_color, nullptr, 0);

		// set vertex shader constants
		D3D11_MAPPED_SUBRESOURCE resource;
		C(device_context->Map(vs_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
		Matrix& m = *(Matrix*)resource.pData;
		m = mat_view_proj.Transpose();
		device_context->Unmap(vs_buffer, 0);

		// set vb
		uint stride = sizeof(ColorVertex),
			offset = 0;
		device_context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
	}

	device_context->Draw(verts.size(), 0);
}

void DebugShader::Draw(const Mesh* mesh, const Matrix& mat_world, Color color)
{
	if(current_vb != mesh->vb)
	{
		// set vb
		uint stride = sizeof(Vec3),
			offset = 0;
		device_context->IASetVertexBuffers(0, 1, &mesh->vb, &stride, &offset);
		device_context->IASetIndexBuffer(mesh->ib, DXGI_FORMAT_R16_UINT, 0);
		device_context->IASetInputLayout(layout);
		device_context->VSSetShader(vertex_shader, nullptr, 0);
		device_context->PSSetShader(pixel_shader, nullptr, 0);
		current_vb = mesh->vb;
	}

	// set vertex shader constants
	D3D11_MAPPED_SUBRESOURCE resource;
	C(device_context->Map(vs_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
	Matrix& m = *(Matrix*)resource.pData;
	m = (mat_world * mat_view_proj).Transpose();
	device_context->Unmap(vs_buffer, 0);

	// set pixel shader constants
	if(color != prev_color)
	{
		prev_color = color;
		C(device_context->Map(ps_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
		Vec4& c = *(Vec4*)resource.pData;
		c = color;
		device_context->Unmap(ps_buffer, 0);
	}

	// draw submeshes
	for(const Mesh::Submesh& sub : mesh->subs)
		device_context->DrawIndexed(sub.tris * 3, sub.first * 3, sub.min_ind);
}
