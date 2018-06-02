#include "EngineCore.h"
#include "DebugDrawer.h"
#include "DebugShader.h"

DebugDrawer::DebugDrawer(Render* render) : render(render), color(Color::Black)
{
}

DebugDrawer::~DebugDrawer()
{
}

void DebugDrawer::Init()
{
	shader.reset(new DebugShader(render));
	shader->Init();
}

void DebugDrawer::Draw(const Matrix& mat_view_proj, delegate<void(DebugDrawer*)> handler)
{
	shader->Prepare(mat_view_proj);
	handler(this);
}

void DebugDrawer::DrawQuad(const Vec3 pos[4])
{
	Vec3* v = shader->Lock();
	*v = pos[0]; ++v;
	*v = pos[1]; ++v;
	*v = pos[2]; ++v;
	*v = pos[1]; ++v;
	*v = pos[2]; ++v;
	*v = pos[3]; ++v;
	shader->Draw(6);
}

void DebugDrawer::SetColor(Color color)
{
	if(color != this->color)
	{
		this->color = color;
		shader->SetColor(color);
	}
}
