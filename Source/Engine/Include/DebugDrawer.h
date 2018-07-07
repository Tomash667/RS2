#pragma once

class DebugDrawer
{
public:
	DebugDrawer(Render* render, ResourceManager* res_mgr);
	~DebugDrawer();
	void Init();
	void Draw(const Matrix& mat_view, const Matrix& mat_view_proj, const Vec3& cam_pos, delegate<void(DebugDrawer*)> handler);
	void DrawLine(const Vec3& from, const Vec3& to, float width);
	void DrawTriangle(const Vec2(&pts)[3], float y);
	void DrawQuad(const Vec3 pos[4]);
	void DrawQuad(const Box2d& box, float y);
	void DrawCube(const Box& box);
	void DrawSphere(const Vec3& pos, float radius);

	void SetColor(Color color);
	void SetWireframe(bool wireframe);

	Color GetColor() { return color; }

private:
	Render* render;
	ResourceManager* res_mgr;
	unique_ptr<DebugShader> shader;
	Mesh* mesh_cube, *mesh_sphere;
	Vec3 cam_pos;
	Matrix mat_view_inv;
	Color color;
};
