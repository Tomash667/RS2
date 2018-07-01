#pragma once

#include "Shader.h"

class SkyShader
{
public:
	SkyShader(Render* render);
	~SkyShader();
	void Init();
	void Draw(Sky* sky, const Matrix& mat_combined);

private:
	void InitInternal();
	void CreateDomeMesh();
	void CreateCelestialMesh();
	void DrawSkydome(const Matrix& mat_combined);
	void DrawCelestialObjects();
	void DrawClouds(const Matrix& mat_combined);

	Render* render;
	ID3D11DeviceContext* device_context;
	ID3D11VertexShader* vertex_shader_dome, *vertex_shader_celestial, *vertex_shader_clouds;
	ID3D11PixelShader* pixel_shader_dome, *pixel_shader_celestial, *pixel_shader_clouds;
	ID3D11InputLayout* layout_dome, *layout_celestial, *layout_clouds;
	ID3D11Buffer* vertex_buffer, *vertex_buffer_celestial, *index_buffer, *index_buffer_celestial, *vs_buffer_dome, *vs_buffer_clouds, *ps_buffer_dome,
		*ps_buffer_celestial, *ps_buffer_clouds;
	ID3D11SamplerState* sampler;
	Sky* sky;
};
