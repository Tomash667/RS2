#pragma once

#include "Shader.h"

class DebugShader
{
public:
	DebugShader(Render* render);
	~DebugShader();
	void Init();
	void Prepare(const Matrix& mat_view_proj);
	void Restore();
	Vec3* Lock(uint vertex_count = 0);
	void Draw(uint vertex_count, bool line_strip = false);
	void Draw(Mesh* mesh, const Matrix& mat_world);

	void SetColor(Color color);
	void SetWireframe(bool wireframe);

private:
	void CreateVertexBuffer();
	void SetLineStrip(bool line_strip);

	Render* render;
	ID3D11DeviceContext* device_context;
	Shader shader;
	ID3D11Buffer* vb, *current_vb;
	Matrix mat_view_proj;
	uint max_vertices;
	bool locked, line_strip;

	static const uint MAX_VERTICES = 6;
};
