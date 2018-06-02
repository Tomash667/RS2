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
	Vec3* Lock();
	void Draw(uint vertex_count);

private:
	Render * render;
	ID3D11DeviceContext* device_context;
	Shader shader;
	ID3D11Buffer* vb;
	bool locked;

	static const uint MAX_VERTICES = 6;
};
