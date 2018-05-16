#pragma once

class Render
{
public:
	enum DepthState
	{
		DEPTH_YES,
		DEPTH_READONLY,
		DEPTH_NO
	};

	Render();
	~Render();
	void Init(Window* window);
	void BeginScene();
	void EndScene();
	void CreateShader(Shader& shader, cstring filename, D3D11_INPUT_ELEMENT_DESC* desc, uint desc_count, uint cbuffer_size[2]);

	void SetClearColor(const Vec4& clear_color) { this->clear_color = clear_color; }
	void SetAlphaBlend(bool enabled);
	void SetDepthState(DepthState state);
	void SetCulling(bool enabled);

	ID3D11Device* GetDevice() { return device; }
	ID3D11DeviceContext* GetDeviceContext() { return device_context; }
	const Vec4& GetClearColor() { return clear_color; }

private:
	void CreateDeviceAndSwapChain();
	void CreateRenderTarget();
	void CreateDepthStencilView();
	void SetViewport();
	void CreateRasterState();
	void CreateBlendState();
	ID3DBlob* CompileShader(cstring filename, bool vertex_shader);

	Window* window;
	IDXGISwapChain* swap_chain;
	ID3D11Device* device;
	ID3D11DeviceContext* device_context;
	ID3D11RenderTargetView* render_target;
	ID3D11DepthStencilView* depth_stencil_view;
	ID3D11DepthStencilState* depth_state, *no_depth_state, *readonly_depth_state;
	ID3D11RasterizerState* raster_state, *no_cull_raster_state;
	ID3D11BlendState* blend_state, *no_blend_state;
	Vec4 clear_color;
	bool vsync, alpha_blend, culling;
	DepthState current_depth_state;
};