#pragma once

#include "Shader.h"

class SkyboxShader
{
public:
	SkyboxShader(Render* render);
	~SkyboxShader();
	void Init();
	void Draw(Mesh* mesh, const Vec3& center, const Matrix& mat_view_proj);
	
private:
	Render* render;
	ID3D11DeviceContext* device_context;
	Shader shader;
	ID3D11SamplerState* sampler;
};
