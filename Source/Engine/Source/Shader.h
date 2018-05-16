#pragma once

struct Shader
{
	Shader();
	~Shader();

	ID3D11VertexShader* vertex_shader;
	ID3D11PixelShader* pixel_shader;
	ID3D11InputLayout* layout;
	ID3D11Buffer* vs_buffer, *ps_buffer;
	uint vertex_stride;
};
