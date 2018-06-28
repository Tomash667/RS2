#include "EngineCore.h"
#include "MeshShader.h"
#include "Render.h"
#include "MeshInstance.h"
#include "Texture.h"
#include "Vertex.h"
#include <d3d11_1.h>
#include "InternalHelper.h"


struct VertexShaderGlobals
{
	Matrix mat_combined;
	Matrix mat_world;
};

struct AnimatedVertexShaderGlobals
{
	Matrix mat_combined;
	Matrix mat_world;
	Matrix mat_bones[32];
};

struct PixelShaderGlobals
{
	Vec4 fog_color;
	Vec4 fog_params;
	Vec4 light_dir;
	Vec4 light_color;
	Vec4 ambient_color;
};

struct PixelShaderPerObject
{
	Vec4 tint;
};


MeshShader::MeshShader(Render* render) : render(render), device_context(nullptr), vertex_shader(nullptr), vertex_shader_animated(nullptr),
pixel_shader(nullptr), layout_mesh(nullptr), layout_animated(nullptr), layout_animated_no_inst(nullptr), vs_buffer(nullptr), vs_buffer_animated(nullptr),
ps_buffer(nullptr), ps_buffer_object(nullptr), sampler(nullptr)
{
}

MeshShader::~MeshShader()
{
	SafeRelease(vertex_shader);
	SafeRelease(vertex_shader_animated);
	SafeRelease(pixel_shader);
	SafeRelease(layout_mesh);
	SafeRelease(layout_animated);
	SafeRelease(layout_animated_no_inst);
	SafeRelease(vs_buffer);
	SafeRelease(vs_buffer_animated);
	SafeRelease(ps_buffer);
	SafeRelease(ps_buffer_object);
	SafeRelease(sampler);
}

void MeshShader::Init()
{
	device_context = render->GetDeviceContext();

	try
	{
		InitInternal();
	}
	catch(cstring err)
	{
		throw Format("Failed to initialize mesh shader: %s", err);
	}
}

void MeshShader::InitInternal()
{
	ID3D11Device* device = render->GetDevice();
	HRESULT result;

	// compile shader to blobs
	ID3DBlob* blob_vs_mesh = render->CompileShader("mesh.hlsl", "vs_mesh", true);
	ID3DBlob* blob_vs_animated = render->CompileShader("mesh.hlsl", "vs_animated", true);
	ID3DBlob* blob_ps = render->CompileShader("mesh.hlsl", "ps_main", false);

	// create shaders
	result = device->CreateVertexShader(blob_vs_mesh->GetBufferPointer(), blob_vs_mesh->GetBufferSize(), nullptr, &vertex_shader);
	if(FAILED(result))
		throw Format("Failed to create vertex shader (%u).", result);

	result = device->CreateVertexShader(blob_vs_animated->GetBufferPointer(), blob_vs_animated->GetBufferSize(), nullptr, &vertex_shader_animated);
	if(FAILED(result))
		throw Format("Failed to create animated vertex shader (%u).", result);

	result = device->CreatePixelShader(blob_ps->GetBufferPointer(), blob_ps->GetBufferSize(), nullptr, &pixel_shader);
	if(FAILED(result))
		throw Format("Failed to create pixel shader (%u).", result);

	// create input layouts
	D3D11_INPUT_ELEMENT_DESC desc_mesh[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	D3D11_INPUT_ELEMENT_DESC desc_animated[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BLENDWEIGHT", 0, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BLENDINDICES", 0, DXGI_FORMAT_R8G8B8A8_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	result = device->CreateInputLayout(desc_mesh, countof(desc_mesh), blob_vs_mesh->GetBufferPointer(), blob_vs_mesh->GetBufferSize(), &layout_mesh);
	if(FAILED(result))
		throw Format("Failed to create mesh layout (%u).", result);

	result = device->CreateInputLayout(desc_animated, countof(desc_animated), blob_vs_animated->GetBufferPointer(), blob_vs_animated->GetBufferSize(),
		&layout_animated);
	if(FAILED(result))
		throw Format("Failed to create animated mesh layout (%u).", result);

	result = device->CreateInputLayout(desc_animated, countof(desc_animated), blob_vs_mesh->GetBufferPointer(), blob_vs_mesh->GetBufferSize(),
		&layout_animated_no_inst);
	if(FAILED(result))
		throw Format("Failed to create animated mesh without instance layout (%u).", result);

	blob_vs_mesh->Release();
	blob_vs_animated->Release();
	blob_ps->Release();

	// create constant buffers
	vs_buffer = render->CreateConstantBuffer(sizeof(VertexShaderGlobals));
	vs_buffer_animated = render->CreateConstantBuffer(sizeof(AnimatedVertexShaderGlobals));
	ps_buffer = render->CreateConstantBuffer(sizeof(PixelShaderGlobals));
	ps_buffer_object = render->CreateConstantBuffer(sizeof(PixelShaderPerObject));

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

void MeshShader::Prepare(const Vec3& fog_color, const Vec3& fog_params, const Vec3& light_dir, const Vec3& light_color, const Vec3& ambient_color)
{
	device_context->PSSetShader(pixel_shader, nullptr, 0);
	ID3D11Buffer* buffers[] = { ps_buffer, ps_buffer_object };
	device_context->PSSetConstantBuffers(0, 2, buffers);
	device_context->PSSetSamplers(0, 1, &sampler);
	current_mode = MODE_NONE;

	D3D11_MAPPED_SUBRESOURCE resource;
	C(device_context->Map(ps_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
	PixelShaderGlobals& psg = *(PixelShaderGlobals*)resource.pData;
	psg.fog_color = Vec4(fog_color, 0);
	psg.fog_params = Vec4(fog_params, 0);
	psg.light_dir = Vec4(light_dir, 0);
	psg.light_color = Vec4(light_color, 0);
	psg.ambient_color = Vec4(ambient_color, 0);
	device_context->Unmap(ps_buffer, 0);
}

void MeshShader::DrawMesh(Mesh* mesh, MeshInstance* mesh_inst, const Matrix& mat_combined, const Matrix& mat_world, const Vec4& tint, int subs)
{
	assert(mesh);

	// change mode
	Mode new_mode;
	if(mesh_inst)
		new_mode = MODE_ANIMATED;
	else if(mesh->IsAnimated())
		new_mode = MODE_ANIMATED_NO_INST;
	else
		new_mode = MODE_MESH;
	if(new_mode != current_mode)
	{
		current_mode = new_mode;
		switch(new_mode)
		{
		case MODE_MESH:
			device_context->IASetInputLayout(layout_mesh);
			device_context->VSSetShader(vertex_shader, nullptr, 0);
			device_context->VSSetConstantBuffers(0, 1, &vs_buffer);
			break;
		case MODE_ANIMATED:
			device_context->IASetInputLayout(layout_animated);
			device_context->VSSetShader(vertex_shader_animated, nullptr, 0);
			device_context->VSSetConstantBuffers(0, 1, &vs_buffer_animated);
			break;
		case MODE_ANIMATED_NO_INST:
			device_context->IASetInputLayout(layout_animated_no_inst);
			device_context->VSSetShader(vertex_shader, nullptr, 0);
			device_context->VSSetConstantBuffers(0, 1, &vs_buffer);
			break;
		}
	}

	// set vertex shader constants
	D3D11_MAPPED_SUBRESOURCE resource;
	if(current_mode != MODE_ANIMATED)
	{
		C(device_context->Map(vs_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
		VertexShaderGlobals& g = *(VertexShaderGlobals*)resource.pData;
		g.mat_combined = mat_combined.Transpose();
		g.mat_world = mat_world.Transpose();
		device_context->Unmap(vs_buffer, 0);
	}
	else
	{
		C(device_context->Map(vs_buffer_animated, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
		AnimatedVertexShaderGlobals& g = *(AnimatedVertexShaderGlobals*)resource.pData;
		g.mat_combined = mat_combined.Transpose();
		g.mat_world = mat_world.Transpose();
		mesh_inst->SetupBones();
		const vector<Matrix>& bones = mesh_inst->GetMatrixBones();
		for(size_t i = 0, count = bones.size(); i < count; ++i)
			g.mat_bones[i] = bones[i].Transpose();
		device_context->Unmap(vs_buffer_animated, 0);
	}

	// set pixel shader constants
	C(device_context->Map(ps_buffer_object, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
	PixelShaderPerObject& pspo = *(PixelShaderPerObject*)resource.pData;
	pspo.tint = tint;
	device_context->Unmap(ps_buffer_object, 0);

	// set buffers
	uint stride = (current_mode == MODE_MESH ? sizeof(Vertex) : sizeof(AniVertex)),
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
