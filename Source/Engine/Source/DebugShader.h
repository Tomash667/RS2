#pragma once

#include "Vertex.h"

class DebugShader
{
public:
	DebugShader(Render* render);
	~DebugShader();
	void Init();
	void Prepare(const Matrix& mat_view_proj);
	void Draw(const vector<ColorVertex>& verts);
	void Draw(const Mesh* mesh, const Matrix& mat_world, Color color);

private:
	void InitInternal();
	void CreateVertexBuffer();

	Render* render;
	ID3D11DeviceContext* device_context;
	ID3D11VertexShader* vertex_shader, *vertex_shader_color;
	ID3D11PixelShader* pixel_shader, *pixel_shader_color;
	ID3D11InputLayout* layout, *layout_color;
	ID3D11Buffer* vs_buffer, *ps_buffer;
	ID3D11Buffer* vb, *current_vb;
	Matrix mat_view_proj;
	uint max_verts;
	Color prev_color;
};
