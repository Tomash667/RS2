#pragma once

class Render
{
public:
	enum DepthState
	{
		DEPTH_YES,
		DEPTH_READONLY,
		DEPTH_NO,
		DEPTH_MAX
	};

	enum BlendState
	{
		BLEND_NO,
		BLEND_NORMAL,
		BLEND_DEST_ONE,
		BLEND_MAX
	};

	enum RasterStateFlags
	{
		RASTER_NO_CULLING = 1 << 0,
		RASTER_WIREFRAME = 1 << 1,
		RASTER_MAX = 1 << 2
	};

	struct DisplayMode
	{
		Int2 size;
		int refresh;
	};

	Render();
	~Render();
	void Prepare();
	void Init(const Int2& wnd_size, void* wnd_handle);
	void BeginScene();
	void EndScene();
	void CreateShader(Shader& shader, cstring filename, D3D11_INPUT_ELEMENT_DESC* desc, uint desc_count, uint cbuffer_size[2]);
	ID3DBlob* CompileShader(cstring filename, cstring entry, bool is_vertex);
	ID3D11Buffer* CreateConstantBuffer(uint size);
	Int2 CheckResolution(const Int2& res);
	void OnChangeResolution(const Int2& wnd_size);

	void SetClearColor(const Vec4& clear_color) { this->clear_color = clear_color; }
	void SetAlphaBlend(BlendState state);
	void SetDepthState(DepthState state);
	void SetCulling(bool enabled);
	void SetVsync(bool vsync) { this->vsync = vsync; }
	void SetWireframe(bool wireframe);

	bool IsVsyncEnabled() { return vsync; }
	bool IsWireframe() { return wireframe; }
	ID3D11Device* GetDevice() { return device; }
	ID3D11DeviceContext* GetDeviceContext() { return device_context; }
	const Vec4& GetClearColor() { return clear_color; }
	const vector<Int2>& GetAvailableResolutions() { return resolutions; }

private:
	void CreateDeviceAndSwapChain(void* wnd_handle);
	void CreateSizeDependentResources();
	void CreateRenderTarget();
	void CreateDepthStencilView();
	void SetViewport();
	void CreateDepthStencilStates();
	void CreateRasterStates();
	void CreateBlendStates();
	void UpdateRasterState();

	IDXGIFactory* factory;
	IDXGIAdapter* adapter;
	IDXGISwapChain* swap_chain;
	ID3D11Device* device;
	ID3D11DeviceContext* device_context;
	ID3D11RenderTargetView* render_target;
	ID3D11DepthStencilView* depth_stencil_view;
	ID3D11DepthStencilState* depth_state[DEPTH_MAX];
	ID3D11RasterizerState* raster_state[RASTER_MAX];
	ID3D11BlendState* blend_state[BLEND_MAX];
	Vec4 clear_color;
	Int2 wnd_size;
	bool vsync, culling, wireframe;
	DepthState current_depth_state;
	BlendState current_blend_state;
	int current_raster_state;
	cstring vs_target_version, ps_target_version;
	vector<Int2> resolutions;
};
