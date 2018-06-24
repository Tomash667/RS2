#pragma once

#include "Shader.h"

class SkyShader
{
public:
	SkyShader(Render* render);
	~SkyShader();
	void Init();
	void Draw(Mesh* mesh, const Vec3& center, const Matrix& mat_view_proj);

private:
	Render* render;
	ID3D11DeviceContext* device_context;
	Shader shader;
	ID3D11SamplerState* sampler;
};
