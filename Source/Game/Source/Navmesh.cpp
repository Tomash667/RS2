#include "GameCore.h"
#include "Navmesh.h"
#include <DebugDrawer.h>
#include <clipper.hpp>
#include <poly2tri.h>

using namespace ClipperLib;

void Navmesh::Reset()
{
	regions.clear();
}

IntPoint ToPoint(const Vec2& p)
{
	return IntPoint(cInt(16 * p.x), cInt(16 * p.y));
}

p2t::Point* ToPoint(const IntPoint& p)
{
	return new p2t::Point(double(p.X) / 16, double(p.Y) / 16);
}

void Navmesh::EndRegion()
{
	if(colliders.empty())
	{
		regions.push_back(region);
		return;
	}

	Clipper clipper;

	// build room polygon
	Path path;
	path.push_back(ToPoint(region.LeftTop()));
	path.push_back(ToPoint(region.RightTop()));
	path.push_back(ToPoint(region.RightBottom()));
	path.push_back(ToPoint(region.LeftBottom()));
	clipper.AddPath(path, ptSubject, true);

	// add colliders
	for(const Box2d& box : colliders)
	{
		path.clear();
		path.push_back(ToPoint(box.LeftTop()));
		path.push_back(ToPoint(box.RightTop()));
		path.push_back(ToPoint(box.RightBottom()));
		path.push_back(ToPoint(box.LeftBottom()));
		clipper.AddPath(path, ptClip, true);
	}

	// clip
	Paths result;
	clipper.Execute(ctDifference, result);

	// for simplicity, first path is polygon, second is hole
	assert(result.size() == 2u);
	// triangulate
	vector<p2t::Point*> polyline;
	for(const IntPoint& pt : result[0])
		polyline.push_back(ToPoint(pt));
	p2t::CDT cdt(polyline);
	vector<p2t::Point*> hole;
	for(auto it = result[1].rbegin(), end = result[1].rend(); it != end; ++it)
		hole.push_back(ToPoint(*it));
	cdt.AddHole(hole);
	cdt.Triangulate();
	vector<p2t::Triangle*> result_triangles = cdt.GetTriangles();

	// copy triangles
	for(p2t::Triangle* tri : result_triangles)
	{
		Triangle tri2;
		for(int i = 0; i < 3; ++i)
		{
			p2t::Point* pt = tri->GetPoint(i);
			tri2.pos[i] = Vec2((float)pt->x, (float)pt->y);
		}
		triangles.push_back(tri2);
	}

	DeleteElements(polyline);
	DeleteElements(hole);
	colliders.clear();
}

void Navmesh::Draw(DebugDrawer* debug_drawer)
{
	const float y = 0.2f;
	debug_drawer->SetColor(Color(0, 128, 255, 128));
	for(Box2d& box : regions)
		debug_drawer->DrawQuad(box, y);
	for(Triangle& tri : triangles)
		debug_drawer->DrawTriangle(tri.pos, y);
	debug_drawer->SetWireframe(true);
	debug_drawer->SetColor(Color(0, 0, 255, 255));
	for(Box2d& box : regions)
		debug_drawer->DrawQuad(box, y);
	for(Triangle& tri : triangles)
		debug_drawer->DrawTriangle(tri.pos, y);
}
