#pragma once

#include "Shader.h"

class MeshShader
{
public:
	MeshShader(Render* render);
	~MeshShader();
	void Init();
	void SetParams(const Vec4& fog_color, const Vec4& fog_params);
	void Prepare();
	void DrawMesh(Mesh* mesh, const Matrix& mat_combined, const Vec3& tint, int subs);

private:
	struct PixelShaderGlobals
	{
		Vec4 fog_color;
		Vec4 fog_params;
		Vec4 tint;
	};

	Render* render;
	ID3D11DeviceContext* device_context;
	Shader shader;
	ID3D11SamplerState* sampler;
	Vec4 fog_color, fog_params;
};
