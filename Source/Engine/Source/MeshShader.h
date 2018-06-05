#pragma once

class MeshShader
{
public:
	MeshShader(Render* render);
	~MeshShader();
	void Init();
	void Prepare(const Vec3& fog_color, const Vec3& fog_params, const Vec3& light_dir, const Vec3& light_color, const Vec3& ambient_color);
	void DrawMesh(Mesh* mesh, MeshInstance* mesh_inst, const Matrix& mat_combined, const Matrix& mat_world, const Vec4& tint, int subs);

private:
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
	ID3D11Buffer* vs_buffer, *vs_buffer_animated, *ps_buffer, *ps_buffer_object;
	ID3D11SamplerState* sampler;
	Mode current_mode;
};
