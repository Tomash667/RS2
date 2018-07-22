#pragma once

#include "Vertex.h"

class DebugDrawer
{
public:
	DebugDrawer(Render* render, ResourceManager* res_mgr);
	~DebugDrawer();
	void Init();
	void BeginBatch();
	void EndBatch();
	void AddVertex(const Vec3& v);
	void AddLine(const Vec3& from, const Vec3& to, float width);
	void Draw(const Matrix& mat_view, const Matrix& mat_view_proj, const Vec3& cam_pos, delegate<void(DebugDrawer*)> handler);
	void DrawLine(const Vec3& from, const Vec3& to, float width);
	void DrawPath(const vector<Vec3>& path, bool closed);
	void DrawPath(const vector<Vec2>& path, float y, bool closed);
	void DrawTriangle(const Vec3(&pts)[3]);
	void DrawTriangle(const Vec2(&pts)[3], float y);
	void DrawQuad(const Vec3 pos[4]);
	void DrawQuad(const Box2d& box, float y);
	void DrawCube(const Box& box);
	void DrawSphere(const Vec3& pos, float radius);

	void SetColor(Color color);
	void SetWireframe(bool wireframe);

	Color GetColor() { return color; }

private:
	void AddLineInternal(ColorVertex* v, const Vec3& from, const Vec3& to, float width);

	Render* render;
	ResourceManager* res_mgr;
	unique_ptr<DebugShader> shader;
	Mesh* mesh_cube, *mesh_sphere;
	Vec3 cam_pos;
	Matrix mat_view_inv;
	Color color;
	Vec4 current_color;
	vector<ColorVertex> verts;
	bool batch;
};
