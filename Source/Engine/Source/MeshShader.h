#pragma once

class MeshShader
{
public:
	MeshShader(Render* render);
	~MeshShader();
	void Init();
	void SetParams(const Vec4& fog_color, const Vec4& fog_params);
	void Prepare();
	void DrawMesh(Mesh* mesh, MeshInstance* mesh_inst, const Matrix& mat_combined, const Vec3& tint, int subs);

private:
	struct VertexShaderGlobals
	{
		Matrix mat_combined;
	};

	struct AnimatedVertexShaderGlobals
	{
		Matrix mat_combined;
		Matrix mat_bones[32];
	};

	struct PixelShaderGlobals
	{
		Vec4 fog_color;
		Vec4 fog_params;
		Vec4 tint;
	};

	enum Mode
	{
		MODE_NONE,
		MODE_MESH,
		MODE_ANIMATED,
		MODE_ANIMATED_NO_INST
	};

	void InitInternal();

	Render* render;
	ID3D11DeviceContext* device_context;
	ID3D11VertexShader* vertex_shader, *vertex_shader_animated;
	ID3D11PixelShader* pixel_shader;
	ID3D11InputLayout* layout_mesh, *layout_animated, *layout_animated_no_inst;
	ID3D11Buffer* vs_buffer, *vs_buffer_animated, *ps_buffer;
	ID3D11SamplerState* sampler;
	Vec4 fog_color, fog_params;
	Mode current_mode;
};
