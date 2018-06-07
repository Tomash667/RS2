#include "EngineCore.h"
#include "Render.h"
#include "Shader.h"
#include "Window.h"
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include "InternalHelper.h"

Render::Render() : swap_chain(nullptr), device(nullptr), device_context(nullptr), render_target(nullptr), depth_stencil_view(nullptr), depth_state(nullptr),
no_depth_state(nullptr), readonly_depth_state(nullptr), raster_state(nullptr), no_cull_raster_state(nullptr), blend_state(nullptr), no_blend_state(nullptr),
clear_color(Color::Black), vsync(true), current_depth_state(DEPTH_YES), alpha_blend(false), culling(true)
{
#ifdef _DEBUG
	vsync = false;
#endif
}

Render::~Render()
{
	SafeRelease(blend_state);
	SafeRelease(no_blend_state);
	SafeRelease(raster_state);
	SafeRelease(no_cull_raster_state);
	SafeRelease(depth_state);
	SafeRelease(no_depth_state);
	SafeRelease(readonly_depth_state);
	SafeRelease(depth_stencil_view);
	SafeRelease(render_target);
	SafeRelease(swap_chain);
	SafeRelease(device_context);

	if(device)
	{
		// write to output directx leaks
		#ifdef _DEBUG
		ID3D11Debug* debug;
		device->QueryInterface(__uuidof(ID3D11Debug), (void**)&debug);
		if(debug)
		{
			debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
			debug->Release();
		}
		#endif

		device->Release();
	}
}

void Render::Init(const Int2& wnd_size, void* wnd_handle)
{
	assert(wnd_handle);
	this->wnd_size = wnd_size;

	CreateDeviceAndSwapChain(wnd_handle);
	CreateSizeDependentResources();
	SetViewport();
	CreateRasterState();
	CreateBlendState();
	CreateDepthStencilState();
}

void Render::CreateDeviceAndSwapChain(void* wnd_handle)
{
	DXGI_SWAP_CHAIN_DESC swap_desc = {};
	swap_desc.BufferCount = 1;
	swap_desc.BufferDesc.Width = wnd_size.x;
	swap_desc.BufferDesc.Height = wnd_size.y;
	swap_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swap_desc.BufferDesc.RefreshRate.Numerator = 0;
	swap_desc.BufferDesc.RefreshRate.Denominator = 1;
	swap_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_desc.OutputWindow = (HWND)wnd_handle;
	swap_desc.SampleDesc.Count = 1;
	swap_desc.SampleDesc.Quality = 0;
	swap_desc.Windowed = true;
	swap_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swap_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_0;
	int flags = 0;
#ifdef _DEBUG
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	HRESULT result = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, &feature_level, 1,
		D3D11_SDK_VERSION, &swap_desc, &swap_chain, &device, nullptr, &device_context);
	if(FAILED(result))
		throw Format("Failed to create device and swap chain (%u).", result);

	// disable alt+enter
	IDXGIDevice* dxgi_device;
	C(device->QueryInterface(IID_PPV_ARGS(&dxgi_device)));

	IDXGIAdapter* adapter;
	C(dxgi_device->GetParent(IID_PPV_ARGS(&adapter)));

	IDXGIFactory* factory;
	C(adapter->GetParent(IID_PPV_ARGS(&factory)));
	C(factory->MakeWindowAssociation(swap_desc.OutputWindow, DXGI_MWA_NO_WINDOW_CHANGES));

	factory->Release();
	adapter->Release();
	dxgi_device->Release();
}

void Render::CreateSizeDependentResources()
{
	CreateRenderTarget();
	CreateDepthStencilView();
	device_context->OMSetRenderTargets(1, &render_target, depth_stencil_view);
	SetViewport();
}

void Render::CreateRenderTarget()
{
	HRESULT result;
	ID3D11Texture2D* back_buffer;
	result = swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&back_buffer);
	if(FAILED(result))
		throw Format("Failed to get back buffer (%u).", result);

	// Create the render target view with the back buffer pointer.
	result = device->CreateRenderTargetView(back_buffer, NULL, &render_target);
	if(FAILED(result))
		throw Format("Failed to create render target view (%u).", result);

	// Release pointer to the back buffer as we no longer need it.
	back_buffer->Release();
}

void Render::CreateDepthStencilView()
{
	// create depth buffer texture
	D3D11_TEXTURE2D_DESC tex_desc = {};

	tex_desc.Width = wnd_size.x;
	tex_desc.Height = wnd_size.y;
	tex_desc.MipLevels = 1;
	tex_desc.ArraySize = 1;
	tex_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	tex_desc.SampleDesc.Count = 1;
	tex_desc.SampleDesc.Quality = 0;
	tex_desc.Usage = D3D11_USAGE_DEFAULT;
	tex_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	tex_desc.CPUAccessFlags = 0;
	tex_desc.MiscFlags = 0;

	ID3D11Texture2D* depth_tex;
	C(device->CreateTexture2D(&tex_desc, nullptr, &depth_tex));

	//==================================================================
	// create depth stencil view from texture
	D3D11_DEPTH_STENCIL_VIEW_DESC view_desc = {};

	view_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	view_desc.Texture2D.MipSlice = 0;

	C(device->CreateDepthStencilView(depth_tex, &view_desc, &depth_stencil_view));

	depth_tex->Release();
}

void Render::SetViewport()
{
	D3D11_VIEWPORT viewport;
	viewport.Width = (float)wnd_size.x;
	viewport.Height = (float)wnd_size.y;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;

	device_context->RSSetViewports(1, &viewport);
}

void Render::CreateDepthStencilState()
{
	// create depth stencil state
	D3D11_DEPTH_STENCIL_DESC desc = {};
	desc.DepthEnable = true;
	desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

	desc.StencilEnable = false;
	desc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	desc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;

	desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;

	desc.BackFace = desc.FrontFace;

	C(device->CreateDepthStencilState(&desc, &depth_state));
	device_context->OMSetDepthStencilState(depth_state, 1);

	//==================================================================
	// create depth stencil state with disabled depth test
	desc.DepthEnable = false;
	desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	C(device->CreateDepthStencilState(&desc, &no_depth_state));

	//==================================================================
	// create readonly depth stencil state
	desc.DepthEnable = true;
	desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	C(device->CreateDepthStencilState(&desc, &readonly_depth_state));
}

void Render::CreateRasterState()
{
	// create default raster state
	D3D11_RASTERIZER_DESC desc;
	desc.AntialiasedLineEnable = false;
	desc.CullMode = D3D11_CULL_BACK;
	desc.DepthBias = 0;
	desc.DepthBiasClamp = 0.0f;
	desc.DepthClipEnable = true;
	desc.FillMode = D3D11_FILL_SOLID;
	desc.FrontCounterClockwise = false;
	desc.MultisampleEnable = false;
	desc.ScissorEnable = false;
	desc.SlopeScaledDepthBias = 0.0f;

	C(device->CreateRasterizerState(&desc, &raster_state));

	device_context->RSSetState(raster_state);

	// create raster state with disabled culling
	desc.CullMode = D3D11_CULL_NONE;

	C(device->CreateRasterizerState(&desc, &no_cull_raster_state));
}

void Render::CreateBlendState()
{
	// get disabled blend state
	device_context->OMGetBlendState(&no_blend_state, nullptr, nullptr);

	// create enabled blend state
	D3D11_BLEND_DESC b_desc = { 0 };
	b_desc.RenderTarget[0].BlendEnable = true;
	b_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	b_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	b_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	b_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	b_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	b_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	b_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	HRESULT result = device->CreateBlendState(&b_desc, &blend_state);
	if(FAILED(result))
		throw Format("Failed to create blend state (%u).", result);
}

void Render::BeginScene()
{
	device_context->ClearRenderTargetView(render_target, clear_color);
	device_context->ClearDepthStencilView(depth_stencil_view, D3D11_CLEAR_DEPTH, 1.f, 0);
}

void Render::EndScene()
{
	C(swap_chain->Present(vsync ? 1 : 0, 0));
}

ID3DBlob* Render::CompileShader(cstring filename, cstring entry, cstring target)
{
	assert(filename && entry && target);

	uint flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
	flags |= D3DCOMPILE_DEBUG;
#endif

	cstring path = Format("Shaders/%s", filename);
	ID3DBlob* shader_blob = nullptr;
	ID3DBlob* error_blob = nullptr;
	HRESULT result = D3DCompileFromFile(ToWString(path), nullptr, nullptr, entry, target, flags, 0, &shader_blob, &error_blob);
	if(FAILED(result))
	{
		SafeRelease(shader_blob);
		if(error_blob)
		{
			cstring err = (cstring)error_blob->GetBufferPointer();
			cstring msg = Format("Failed to compile '%s' function '%s': %s (code %u).", path, entry, err, result);
			error_blob->Release();
			throw msg;
		}
		else
			throw Format("Failed to compile '%s' function '%s' (code %u).", path, entry, result);
	}

	if(error_blob)
	{
		cstring err = (cstring)error_blob->GetBufferPointer();
		Warn("Shader '%s' warnings: %s", path, err);
		error_blob->Release();
	}

	return shader_blob;
}

void Render::CreateShader(Shader& shader, cstring filename, D3D11_INPUT_ELEMENT_DESC* desc, uint desc_count, uint cbuffer_size[2])
{
	// create vertex shader
	CPtr<ID3DBlob> vs_buf = CompileShader(filename, "vs_main", "vs_5_0");
	HRESULT result = device->CreateVertexShader(vs_buf->GetBufferPointer(), vs_buf->GetBufferSize(), nullptr, &shader.vertex_shader);
	if(FAILED(result))
		throw Format("Failed to create vertex shader '%s' (%u).", filename, result);

	// create pixel shader
	CPtr<ID3DBlob> ps_buf = CompileShader(filename, "ps_main", "ps_5_0");
	result = device->CreatePixelShader(ps_buf->GetBufferPointer(), ps_buf->GetBufferSize(), nullptr, &shader.pixel_shader);
	if(FAILED(result))
		throw Format("Failed to create pixel shader '%s' (%u).", filename, result);

	// create layout
	result = device->CreateInputLayout(desc, desc_count, vs_buf->GetBufferPointer(), vs_buf->GetBufferSize(), &shader.layout);
	if(FAILED(result))
		throw Format("Failed to create input layout '%s' (%u).", filename, result);

	// create cbuffers
	try
	{
		if(cbuffer_size[0] > 0)
			shader.vs_buffer = CreateConstantBuffer(cbuffer_size[0]);

		if(cbuffer_size[1] > 0)
			shader.ps_buffer = CreateConstantBuffer(cbuffer_size[1]);
	}
	catch(cstring err)
	{
		throw Format("Failed to create shader '%s': %s", filename, err);
	}

	vs_buf->Release();
	ps_buf->Release();
}

ID3D11Buffer* Render::CreateConstantBuffer(uint size)
{
	D3D11_BUFFER_DESC cb_desc;
	cb_desc.Usage = D3D11_USAGE_DYNAMIC;
	cb_desc.ByteWidth = size;
	cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cb_desc.MiscFlags = 0;
	cb_desc.StructureByteStride = 0;

	ID3D11Buffer* buffer;
	HRESULT result = device->CreateBuffer(&cb_desc, NULL, &buffer);
	if(FAILED(result))
		throw Format("Failed to create constant buffer (size:%u; code:%u).", size, result);

	return buffer;
}

void Render::SetAlphaBlend(bool enabled)
{
	if(enabled != alpha_blend)
	{
		alpha_blend = enabled;
		device_context->OMSetBlendState(enabled ? blend_state : no_blend_state, nullptr, 0xFFFFFFFF);
	}
}

void Render::SetDepthState(DepthState state)
{
	if(state != current_depth_state)
	{
		current_depth_state = state;
		ID3D11DepthStencilState* depth_stencil_state;
		switch(current_depth_state)
		{
		case DEPTH_YES:
		default:
			depth_stencil_state = depth_state;
			break;
		case DEPTH_READONLY:
			depth_stencil_state = readonly_depth_state;
			break;
		case DEPTH_NO:
			depth_stencil_state = no_depth_state;
			break;
		}
		device_context->OMSetDepthStencilState(depth_stencil_state, 0);
	}
}

void Render::SetCulling(bool enabled)
{
	if(culling != enabled)
	{
		culling = enabled;
		device_context->RSSetState(enabled ? raster_state : no_cull_raster_state);
	}
}

void Render::OnSizeChange(const Int2& wnd_size)
{
	if(this->wnd_size == wnd_size || !swap_chain)
		return;

	this->wnd_size = wnd_size;

	render_target->Release();
	depth_stencil_view->Release();

	DXGI_SWAP_CHAIN_DESC desc = {};
	swap_chain->GetDesc(&desc);
	C(swap_chain->ResizeBuffers(1, wnd_size.x, wnd_size.y, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	CreateSizeDependentResources();
}
