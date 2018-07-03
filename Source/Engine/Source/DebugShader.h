#pragma once

#include "Shader.h"

class DebugShader
{
public:
	DebugShader(Render* render);
	~DebugShader();
	void Init();
	void Prepare(const Matrix& mat_view_proj);
	void SetColor(Color color);
	void SetWireframe(bool wireframe);
	Vec3* Lock();
	void Draw(uint vertex_count);
	void Draw(Mesh* mesh, const Matrix& mat_world);

private:
	Render * render;
	ID3D11DeviceContext* device_context;
	Shader shader;
	ID3D11Buffer* vb, *current_vb;
	Matrix mat_view_proj;
	bool locked;

	static const uint MAX_VERTICES = 6;
};
