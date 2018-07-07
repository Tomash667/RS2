#include "EngineCore.h"
#include "DebugDrawer.h"
#include "DebugShader.h"
#include "ResourceManager.h"

DebugDrawer::DebugDrawer(Render* render, ResourceManager* res_mgr) : render(render), res_mgr(res_mgr), color(Color::Black)
{
}

DebugDrawer::~DebugDrawer()
{
}

void DebugDrawer::Init()
{
	shader.reset(new DebugShader(render));
	shader->Init();

	mesh_cube = res_mgr->GetMesh("engine/cube.qmsh");
	mesh_sphere = res_mgr->GetMesh("engine/sphere.qmsh");
}

void DebugDrawer::Draw(const Matrix& mat_view, const Matrix& mat_view_proj, const Vec3& cam_pos, delegate<void(DebugDrawer*)> handler)
{
	assert(handler);
	mat_view_inv = mat_view.Inverse();
	this->cam_pos = cam_pos;
	shader->Prepare(mat_view_proj);
	handler(this);
	shader->SetWireframe(false);
}

// https://www.gamedev.net/forums/topic/617595-solved-thick-constant-width-lines-using-quads/
// can be used to draw lines with selected pixel width
// fix for line point behind camera
void DebugDrawer::DrawLine(const Vec3& from, const Vec3& to, float width)
{
	Vec3* v = shader->Lock();

	width /= 2;

	Vec3 line_dir = from - to;
	Vec3 quad_normal = cam_pos - (to + from) / 2;
	Vec3 extrude_dir = line_dir.Cross(quad_normal).Normalize();

	v[0] = from + extrude_dir * width;
	v[1] = from - extrude_dir * width;
	v[2] = to + extrude_dir * width;
	v[3] = v[1];
	v[4] = v[2];
	v[5] = to - extrude_dir * width;

	shader->Draw(6);
}

void DebugDrawer::DrawTriangle(const Vec2(&pts)[3], float y)
{
	Vec3* v = shader->Lock();

	for(int i = 0; i < 3; ++i)
		v[i] = pts[i].XZ(y);

	shader->Draw(3);
}

void DebugDrawer::DrawQuad(const Vec3 pos[4])
{
	Vec3* v = shader->Lock();

	v[0] = pos[0];
	v[1] = pos[1];
	v[2] = pos[2];
	v[3] = pos[1];
	v[4] = pos[2];
	v[5] = pos[3];

	shader->Draw(6);
}

void DebugDrawer::DrawQuad(const Box2d& box, float y)
{
	Vec3* v = shader->Lock();

	v[0] = box.LeftTop().XZ(y);
	v[1] = box.RightTop().XZ(y);
	v[2] = box.LeftBottom().XZ(y);
	v[3] = v[2];
	v[4] = v[1];
	v[5] = box.RightBottom().XZ(y);

	shader->Draw(6);
}

void DebugDrawer::DrawCube(const Box& box)
{
	shader->Draw(mesh_cube, Matrix::Scale(box.Size() / 2) * Matrix::Translation(box.Midpoint()));
}

void DebugDrawer::DrawSphere(const Vec3& pos, float radius)
{
	shader->Draw(mesh_sphere, Matrix::Scale(radius) * Matrix::Translation(pos));
}

void DebugDrawer::SetColor(Color color)
{
	if(color != this->color)
	{
		this->color = color;
		shader->SetColor(color);
	}
}

void DebugDrawer::SetWireframe(bool wireframe)
{
	shader->SetWireframe(wireframe);
}
