#include "GameCore.h"
#include "Navmesh.h"
#include <DebugDrawer.h>
#include <clipper.hpp>
#include <poly2tri.h>

using namespace ClipperLib;

void Navmesh::Reset()
{
	triangles.clear();
}

IntPoint ToIntPoint(const Vec2& p)
{
	return IntPoint(cInt(16 * p.x), cInt(16 * p.y));
}

p2t::Point* ToPoint(const IntPoint& p)
{
	return new p2t::Point(double(p.X) / 16, double(p.Y) / 16);
}

p2t::Point* ToPoint(const Vec2& p)
{
	return new p2t::Point(p.x, p.y);
}

void Navmesh::EndRegion()
{
	colliders.clear();
	outlines.push_back(*outline);
	return;

	if(colliders.empty())
	{
		vector<p2t::Point*> polyline;
		for(const Vec2& pt : *outline)
			polyline.push_back(ToPoint(pt));
		Triangulate(&polyline, nullptr);
		return;
	}

	Clipper clipper;

	// build room polygon
	Path path;
	for(const Vec2& pt : *outline)
		path.push_back(ToIntPoint(pt));
	clipper.AddPath(path, ptSubject, true);

	// add colliders
	for(const Box2d& box : colliders)
	{
		path.clear();
		path.push_back(ToIntPoint(box.LeftTop()));
		path.push_back(ToIntPoint(box.RightTop()));
		path.push_back(ToIntPoint(box.RightBottom()));
		path.push_back(ToIntPoint(box.LeftBottom()));
		clipper.AddPath(path, ptClip, true);
	}

	// clip
	Paths result;
	clipper.Execute(ctDifference, result);

	// for simplicity, first path is polygon, second is hole
	assert(result.size() == 2u);
	vector<p2t::Point*> polyline, hole;
	for(const IntPoint& pt : result[0])
		polyline.push_back(ToPoint(pt));
	for(auto it = result[1].rbegin(), end = result[1].rend(); it != end; ++it)
		hole.push_back(ToPoint(*it));

	Triangulate(&polyline, &hole);
	colliders.clear();
}

void Navmesh::Triangulate(vector<p2t::Point*>* polyline, vector<p2t::Point*>* hole)
{
	// traingulate
	p2t::CDT cdt(*polyline);
	if(hole)
		cdt.AddHole(*hole);
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

	// cleanup
	DeleteElements(*polyline);
	if(hole)
		DeleteElements(*hole);
}

float my_dt;

void Navmesh::Draw(DebugDrawer* debug_drawer)
{
	const float y = 0.2f;
	/*debug_drawer->SetColor(Color(0, 128, 255, 128));
	for(Triangle& tri : triangles)
		debug_drawer->DrawTriangle(tri.pos, y);
	debug_drawer->SetWireframe(true);*/
	debug_drawer->SetColor(Color(0, 0, 255, 255));
	//for(Triangle& tri : triangles)
	//	debug_drawer->DrawTriangle(tri.pos, y);

	my_dt += dt;
	static int c = 0;
	if(my_dt >= 1.f)
	{
		//Info("tick");
		my_dt -= 1.f;
		++c;
		if(c == 3)
			c = 0;
	}
	for(uint i = 0; i < outlines.size(); ++i)
	{
		//if(i%3 == c)
			debug_drawer->DrawPath(outlines[i], y, true);
	}
	
	//for(vector<Vec2>& outline : outlines)
	//	debug_drawer->DrawPath(outline, y, true);
}
