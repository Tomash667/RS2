#include "GameCore.h"
#include "Navmesh.h"
#include <DebugDrawer.h>
#include <Recast.h>
#include "Unit.h"
// FIXME
#include <Input.h>

Navmesh::Navmesh() : ctx(nullptr)
{

}

Navmesh::~Navmesh()
{

}

void Navmesh::Init(float map_size)
{
	rcConfig cfg = {};
	cfg.cs = Unit::radius; // cell size
	cfg.ch = 0.25f; // cell height
	cfg.walkableSlopeAngle = 45.f;
	cfg.walkableHeight = (int)ceilf(Unit::height / cfg.ch);
	cfg.walkableClimb = 1; // in cells
	cfg.walkableRadius = 1; // in cell size (Unit has size of 1 cell, needs more testing)
	cfg.maxEdgeLen = 12; // ????? FIXME
	cfg.maxSimplificationError = 1.3f; // ^^^^^^
	cfg.minRegionArea = rcSqr(8); // ^
	cfg.mergeRegionArea = rcSqr(20); // ^
	cfg.maxVertsPerPoly = 6; // ^
	cfg.detailSampleDist = cfg.cs * 6; // m_detailSampleDist < 0.9f ? 0 : m_cellSize * m_detailSampleDist
	cfg.detailSampleMaxError = cfg.ch * 1.f; // ^
	rcVcopy(cfg.bmin, Vec3(0, 0, 0));
	rcVcopy(cfg.bmax, Vec3(map_size, 2.f, map_size));
	rcCalcGridSize(cfg.bmin, cfg.bmax, cfg.cs, &cfg.width, &cfg.height);

	// Allocate voxel heightfield where we rasterize our input data to.
	rcHeightfield* solid = rcAllocHeightfield();
	rcCreateHeightfield(&ctx, *solid, cfg.width, cfg.height, cfg.bmin, cfg.bmax, cfg.cs, cfg.ch);

	const int n_tris = 2;
	Vec3 verts[] = {
		Vec3(0,0,0),
		Vec3(map_size,0,0),
		Vec3(0,0,map_size),
		Vec3(map_size,0,map_size)
	};
	const int n_verts = 4;
	int tris[] = {
		0, 1, 2,
		2, 1, 3
	};

	// Allocate array that can hold triangle area types.
	// If you have multiple meshes you need to process, allocate
	// and array which can hold the max number of triangles you need to process.
	vector<byte> triareas;
	triareas.resize(n_tris);

	// Find triangles which are walkable based on their slope and rasterize them.
	// If your input data is multiple meshes, you can transform them here, calculate
	// the are type for each of the meshes and rasterize them.
	memset(triareas.data(), 0, n_tris * sizeof(byte));
	rcMarkWalkableTriangles(&ctx, cfg.walkableSlopeAngle, (float*)verts, n_verts, tris, n_tris, triareas.data());
	rcRasterizeTriangles(&ctx, (float*)verts, triareas.data(), n_tris, *solid, cfg.walkableClimb);

	rcFreeHeightField(solid);
}

void Navmesh::Reset()
{
	triangles.clear();
}

void Navmesh::EndRegion()
{
	

	/*if(colliders.empty())
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
	colliders.clear();*/
}

/*void Navmesh::Triangulate(vector<p2t::Point*>* polyline, vector<p2t::Point*>* hole)
{
	// traingulate
	p2t::CDT cdt(*polyline);
	if(hole)
		cdt.AddHole(*hole);
	vector<p2t::Point*> added_points;
	for(Vec2& pt : points)
	{
		p2t::Point* p = ToPoint(pt);
		cdt.AddPoint(p);
		added_points.push_back(p);
	}
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
	DeleteElements(added_points);
	points.clear();
}*/

float my_dt;

void Navmesh::Draw(DebugDrawer* debug_drawer)
{
	const float y = 0.2f;
	debug_drawer->SetColor(Color(0, 128, 255, 128));
	for(Triangle& tri : triangles)
		debug_drawer->DrawTriangle(tri.pos, y);
	debug_drawer->SetWireframe(true);
	debug_drawer->SetColor(Color(0, 0, 255, 255));
	for(Triangle& tri : triangles)
		debug_drawer->DrawTriangle(tri.pos, y);

	//my_dt += dt;
	//static int c = 13;
	//if(my_dt >= 1.f)
	//{
	//	//Info("tick");
	//	my_dt -= 1.f;
	//	//++c;
	//	if(c == 3)
	//		c = 0;
	//}
	//if(input->Pressed(Key::F5))
	//{
	//	--c;
	//	if(c == -1)
	//		c = outlines.size() - 1;
	//}
	//if(input->Pressed(Key::F6))
	//{
	//	++c;
	//	if(c == outlines.size())
	//		c = 0;
	//}
	//for(uint i = 0; i < outlines.size(); ++i)
	//{
	//	//if(i == c)
	//		debug_drawer->DrawPath(outlines[i], y, true);
	//}
	
	//for(vector<Vec2>& outline : outlines)
	//	debug_drawer->DrawPath(outline, y, true);
}
