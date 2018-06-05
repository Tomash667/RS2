#pragma once

class DebugDrawer
{
public:
	DebugDrawer(Render* render, ResourceManager* res_mgr);
	~DebugDrawer();
	void Init();
	void Draw(const Matrix& mat_view, const Matrix& mat_view_proj, const Vec3& cam_pos, delegate<void(DebugDrawer*)> handler);
	void DrawLine(const Vec3& from, const Vec3& to, float width);
	void DrawQuad(const Vec3 pos[4]);
	void DrawSphere(const Vec3& pos, float radius);

	void SetColor(Color color);

	Color GetColor() { return color; }

private:
	Render* render;
	ResourceManager* res_mgr;
	unique_ptr<DebugShader> shader;
	Mesh* mesh_sphere;
	Vec3 cam_pos;
	Matrix mat_view_inv;
	Color color;
};
