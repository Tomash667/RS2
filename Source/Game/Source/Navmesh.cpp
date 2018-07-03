#include "GameCore.h"
#include "Navmesh.h"
#include <DebugDrawer.h>

void Navmesh::Reset()
{
	regions.clear();
}

void Navmesh::StartRegion(const Box2d& box)
{
	regions.push_back(box);
}

void Navmesh::Draw(DebugDrawer* debug_drawer)
{
	debug_drawer->SetColor(Color(0, 128, 255, 128));
	for(Box2d& box : regions)
		debug_drawer->DrawQuad(box, 1.f);
	debug_drawer->SetWireframe(true);
	debug_drawer->SetColor(Color(0, 0, 255, 196));
	for(Box2d& box : regions)
		debug_drawer->DrawQuad(box, 1.f);
}
