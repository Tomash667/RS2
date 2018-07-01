#pragma once

#include "Shader.h"

class SkyboxShader
{
public:
	SkyboxShader(Render* render);
	~SkyboxShader();
	void Init();
	void Draw(Mesh* mesh, const Matrix& mat_combined);
	
private:
	Render* render;
	ID3D11DeviceContext* device_context;
	Shader shader;
	ID3D11SamplerState* sampler;
};
