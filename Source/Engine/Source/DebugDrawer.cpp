#include "EngineCore.h"
#include "DebugDrawer.h"
#include "DebugShader.h"
#include "ResourceManager.h"
#include "Render.h"

DebugDrawer::DebugDrawer(Render* render, ResourceManager* res_mgr) : render(render), res_mgr(res_mgr), color(Color::Black), batch(false)
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

void DebugDrawer::BeginBatch()
{
	assert(!batch);
	verts.clear();
	batch = true;
}

void DebugDrawer::EndBatch()
{
	assert(batch);
	if(!verts.empty())
		shader->Draw(verts);
	batch = false;
}

void DebugDrawer::AddVertex(const Vec3& pos)
{
	assert(batch);
	verts.push_back({ pos, current_color });
}

void DebugDrawer::AddLine(const Vec3& from, const Vec3& to, float width)
{
	assert(batch);
	uint offset = verts.size();
	verts.resize(offset + 6);
	ColorVertex* v = verts.data() + offset;
	AddLineInternal(v, from, to, width);
}

// https://www.gamedev.net/forums/topic/617595-solved-thick-constant-width-lines-using-quads/
// can be used to draw lines with selected pixel width
// fix for line point behind camera
void DebugDrawer::AddLineInternal(ColorVertex* v, const Vec3& from, const Vec3& to, float width)
{
	width /= 2;

	Vec3 line_dir = from - to;
	Vec3 quad_normal = cam_pos - (to + from) / 2;
	Vec3 extrude_dir = line_dir.Cross(quad_normal).Normalize();

	v[0].pos = from + extrude_dir * width;
	v[1].pos = from - extrude_dir * width;
	v[2].pos = to + extrude_dir * width;
	v[3].pos = v[1].pos;
	v[4].pos = v[2].pos;
	v[5].pos = to - extrude_dir * width;
	for(int i = 0; i<6; ++i)
		v[i].color = current_color;
}

void DebugDrawer::Draw(const Matrix& mat_view, const Matrix& mat_view_proj, const Vec3& cam_pos, delegate<void(DebugDrawer*)> handler)
{
	assert(handler);
	mat_view_inv = mat_view.Inverse();
	this->cam_pos = cam_pos;
	shader->Prepare(mat_view_proj);
	handler(this);
	render->SetWireframe(false);
}

// https://www.gamedev.net/forums/topic/617595-solved-thick-constant-width-lines-using-quads/
// can be used to draw lines with selected pixel width
// fix for line point behind camera
void DebugDrawer::DrawLine(const Vec3& from, const Vec3& to, float width)
{
	assert(!batch);
	verts.resize(6);
	AddLineInternal(verts.data(), from, to, width);
	shader->Draw(verts);
}

void DebugDrawer::DrawPath(const vector<Vec3>& path, bool closed)
{
	assert(!batch);
	assert(path.size() >= 2u);
	uint edges = path.size();
	if(closed)
		++edges;
	verts.resize(edges * 2);
	ColorVertex* v = verts.data();
	for(uint i = 1, count = path.size(); i < count; ++i)
	{
		v->pos = path[i - 1];
		v->color = current_color;
		++v;
		v->pos = path[i];
		v->color = current_color;
		++v;
	}
	if(closed)
	{
		v->pos = path.back();
		v->color = current_color;
		++v;
		v->pos = path.front();
		v->color = current_color;
	}
	shader->Draw(verts);
}

void DebugDrawer::DrawPath(const vector<Vec2>& path, float y, bool closed)
{
	assert(!batch);
	assert(path.size() >= 2u);
	uint edges = path.size();
	if(closed)
		++edges;
	verts.resize(edges );
	ColorVertex* v = verts.data();
	for(uint i = 1, count = path.size(); i < count; ++i)
	{
		v->pos = path[i - 1].XZ(y);
		v->color = current_color;
		++v;
		v->pos = path[i].XZ(y);
		v->color = current_color;
		++v;
	}
	if(closed)
	{
		v->pos = path.back().XZ(y);
		v->color = current_color;
		++v;
		v->pos = path.front().XZ(y);
		v->color = current_color;
	}
	shader->Draw(verts);
}

void DebugDrawer::DrawTriangle(const Vec3(&pts)[3])
{
	assert(!batch);
	verts.resize(3);
	ColorVertex* v = verts.data();
	for(int i = 0; i < 3; ++i)
	{
		v[i].pos = pts[i];
		v[i].color = current_color;
	}
	shader->Draw(verts);
}

void DebugDrawer::DrawTriangle(const Vec2(&pts)[3], float y)
{
	assert(!batch);
	verts.resize(3);
	ColorVertex* v = verts.data();
	for(int i = 0; i < 3; ++i)
	{
		v[i].pos = pts[i].XZ(y);
		v[i].color = current_color;
	}
	shader->Draw(verts);
}

void DebugDrawer::DrawQuad(const Vec3 pos[4])
{
	assert(!batch);
	verts.resize(6);
	ColorVertex* v = verts.data();
	v[0].pos = pos[0];
	v[1].pos = pos[1];
	v[2].pos = pos[2];
	v[3].pos = pos[1];
	v[4].pos = pos[2];
	v[5].pos = pos[3];
	for(int i = 0; i < 6; ++i)
		v[i].color = current_color;
	shader->Draw(verts);
}

void DebugDrawer::DrawQuad(const Box2d& box, float y)
{
	assert(!batch);
	verts.resize(6);
	ColorVertex* v = verts.data();
	v[0].pos = box.LeftTop().XZ(y);
	v[1].pos = box.RightTop().XZ(y);
	v[2].pos = box.LeftBottom().XZ(y);
	v[3].pos = v[2].pos;
	v[4].pos = v[1].pos;
	v[5].pos = box.RightBottom().XZ(y);
	for(int i = 0; i < 6; ++i)
		v[i].color = current_color;
	shader->Draw(verts);
}

void DebugDrawer::DrawCube(const Box& box)
{
	assert(!batch);
	shader->Draw(mesh_cube, Matrix::Scale(box.Size() / 2) * Matrix::Translation(box.Midpoint()), color);
}

void DebugDrawer::DrawSphere(const Vec3& pos, float radius)
{
	assert(!batch);
	shader->Draw(mesh_sphere, Matrix::Scale(radius) * Matrix::Translation(pos), color);
}

void DebugDrawer::SetColor(Color color)
{
	if(color != this->color)
	{
		this->color = color;
		current_color = color;
	}
}

void DebugDrawer::SetWireframe(bool wireframe)
{
	render->SetWireframe(wireframe);
}
