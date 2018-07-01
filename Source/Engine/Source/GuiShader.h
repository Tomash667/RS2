#pragma once

#include "Shader.h"
#include "Vertex.h"

class GuiShader
{
public:
	static const uint MaxVertex = 256 * 3;

	GuiShader(Render* render);
	~GuiShader();
	void Init();
	void Prepare();
	GuiVertex* Lock();
	void Draw(Texture* tex, uint vertex_count);

	void SetWindowSize(const Int2& wnd_size) { this->wnd_size = wnd_size; }

	Texture* GetEmptyTexture() { return empty_texture; }

private:
	Render* render;
	ID3D11DeviceContext* device_context;
	Shader shader;
	ID3D11SamplerState* sampler;
	ID3D11Buffer* vb;
	Texture* empty_texture;
	Int2 wnd_size;
	bool locked;
};
