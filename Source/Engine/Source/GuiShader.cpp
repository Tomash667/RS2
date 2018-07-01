#include "EngineCore.h"
#include "GuiShader.h"
#include "Render.h"
#include "Texture.h"
#include <d3d11_1.h>
#include "InternalHelper.h"

struct ShaderGlobals
{
	Vec2 size;
	Vec2 _pad;
};

GuiShader::GuiShader(Render* render) : render(render), device_context(nullptr), sampler(nullptr), vb(nullptr), empty_texture(nullptr), locked(false)
{
}

GuiShader::~GuiShader()
{
	SafeRelease(sampler);
	SafeRelease(vb);
	delete empty_texture;
}

void GuiShader::Init()
{
	device_context = render->GetDeviceContext();

	// create shader
	D3D11_INPUT_ELEMENT_DESC desc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	uint cbuffer_size[] = { sizeof(ShaderGlobals), 0 };
	render->CreateShader(shader, "gui.hlsl", desc, countof(desc), cbuffer_size);
	shader.vertex_stride = sizeof(GuiVertex);

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
		throw Format("Failed to create gui sampler state (%u).", result);

	// create vertex buffer
	D3D11_BUFFER_DESC v_desc;
	v_desc.Usage = D3D11_USAGE_DYNAMIC;
	v_desc.ByteWidth = MaxVertex * sizeof(GuiVertex);
	v_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	v_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	v_desc.MiscFlags = 0;
	v_desc.StructureByteStride = 0;

	result = render->GetDevice()->CreateBuffer(&v_desc, nullptr, &vb);
	if(FAILED(result))
		throw Format("Failed to create gui vertex buffer (%u).", result);

	// create empty texture
	D3D11_TEXTURE2D_DESC t_desc = { 0 };
	t_desc.Width = 1;
	t_desc.Height = 1;
	t_desc.MipLevels = 1;
	t_desc.ArraySize = 1;
	t_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	t_desc.SampleDesc.Count = 1;
	t_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	t_desc.Usage = D3D11_USAGE_DEFAULT;

	Vec4 color = Color::White;
	D3D11_SUBRESOURCE_DATA t_sub_data = { 0 };
	t_sub_data.pSysMem = &color;
	t_sub_data.SysMemPitch = sizeof(Vec4);

	ID3D11Texture2D* tex;
	C(render->GetDevice()->CreateTexture2D(&t_desc, &t_sub_data, &tex));

	ID3D11ShaderResourceView* tex_view;
	D3D11_SHADER_RESOURCE_VIEW_DESC tv_desc = { 0 };
	tv_desc.Format = t_desc.Format;
	tv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	tv_desc.Texture2D.MipLevels = 1;

	C(render->GetDevice()->CreateShaderResourceView(tex, &tv_desc, &tex_view));

	tex->Release();

	empty_texture = new Texture("EmptyTexture", tex_view);
}

void GuiShader::Prepare()
{
	render->SetAlphaBlend(Render::BLEND_NORMAL);
	render->SetDepthState(Render::DEPTH_NO);
	render->SetCulling(true);

	device_context->IASetInputLayout(shader.layout);
	device_context->VSSetShader(shader.vertex_shader, nullptr, 0);
	device_context->VSSetConstantBuffers(0, 1, &shader.vs_buffer);
	device_context->PSSetShader(shader.pixel_shader, nullptr, 0);
	device_context->PSSetSamplers(0, 1, &sampler);

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	C(device_context->Map(shader.vs_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));
	ShaderGlobals& g = *(ShaderGlobals*)mappedResource.pData;
	g.size = Vec2(wnd_size);
	device_context->Unmap(shader.vs_buffer, 0);
}

GuiVertex* GuiShader::Lock()
{
	assert(!locked);
	locked = true;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	C(device_context->Map(vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));
	return (GuiVertex*)mappedResource.pData;
}

void GuiShader::Draw(Texture* tex, uint count)
{
	assert(tex && locked && count <= MaxVertex);

	device_context->Unmap(vb, 0);
	locked = false;

	if(count == 0)
		return;

	uint stride = shader.vertex_stride,
		offset = 0;
	device_context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);

	device_context->PSSetShaderResources(0, 1, &tex->tex);
	device_context->Draw(count, 0);
}
