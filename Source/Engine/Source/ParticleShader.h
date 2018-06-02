#pragma once

#include "Shader.h"

class ParticleShader
{
public:
	ParticleShader(Render* render);
	~ParticleShader();
	void Init();
	void Prepare(const Matrix& mat_view, const Matrix& mat_view_proj);
	void Draw(ParticleEmitter* pe);

private:
	Render* render;
	ID3D11DeviceContext* device_context;
	Shader shader;
	ID3D11SamplerState* sampler;
	ID3D11Buffer* vb;
	uint vb_size;
	Matrix mat_view_inv;
};
