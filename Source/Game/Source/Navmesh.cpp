#include "GameCore.h"
#include "Navmesh.h"
#include <DebugDrawer.h>
#include <Recast.h>
#include <DetourNavMesh.h>
#include <DetourNavMeshBuilder.h>
#include "Unit.h"
// FIXME
#include <Input.h>

enum PolyArea
{
	POLYAREA_GROUND
};

enum PolyFlags
{
	POLYFLAGS_WALK = 1 << 0
};

struct BuildContext : rcContext
{
	BuildContext() : rcContext(false)
	{
#ifdef _DEBUG
		enableLog(true);
#endif
	}

	void doLog(const rcLogCategory category, const char* msg, const int) override
	{
		Logger::Level level;
		switch(category)
		{
		default:
		case RC_LOG_PROGRESS:
			level = Logger::L_INFO;
			break;
		case RC_LOG_WARNING:
			level = Logger::L_WARN;
			break;
		case RC_LOG_ERROR:
			level = Logger::L_ERROR;
			break;
		}
		Logger::Get()->Log(level, msg);
	}
};

Navmesh::Navmesh() : solid(nullptr), chf(nullptr), cset(nullptr), pmesh(nullptr), dmesh(nullptr), navmesh(nullptr)
{
	nav_query = dtAllocNavMeshQuery();

	filter.setIncludeFlags(POLYFLAGS_WALK);
	filter.setExcludeFlags(0);
}

Navmesh::~Navmesh()
{
	Reset();
	dtFreeNavMeshQuery(nav_query);
}

void Navmesh::Reset()
{
	rcFreeHeightField(solid);
	solid = nullptr;
	rcFreeCompactHeightfield(chf);
	chf = nullptr;
	rcFreeContourSet(cset);
	cset = nullptr;
	rcFreePolyMesh(pmesh);
	pmesh = nullptr;
	rcFreePolyMeshDetail(dmesh);
	dmesh = nullptr;
	dtFreeNavMesh(navmesh);
	navmesh = nullptr;
}

bool Navmesh::Build(float map_size)
{
	// FIXME tmp
	have_path = false;

	// FIXME
	map_size = 10.f;
	Info("Building navmesh...");
	Timer t;

	Reset();


	const float agent_radius = Unit::radius;
	const float agent_height = Unit::height;
	const float agent_climb = 0.25f;

	BuildContext ctx;

	//
	// Step 1. Initialize build config.
	//

	// Init configuration (values from RecastDemo...)
	const float detailSampleDist = 6.f;
	rcConfig cfg = {};
	cfg.cs = Unit::radius; // cell size
	cfg.ch = 0.25f; // cell height
	cfg.walkableSlopeAngle = 45.f;
	cfg.walkableHeight = (int)ceilf(agent_height / cfg.ch);
	cfg.walkableClimb = 1; // in cells
	cfg.walkableRadius = 1; // in cell size (Unit has size of 1 cell, needs more testing)
	cfg.maxEdgeLen = 12;
	cfg.maxSimplificationError = 1.3f;
	cfg.minRegionArea = rcSqr(8);
	cfg.mergeRegionArea = rcSqr(20);
	cfg.maxVertsPerPoly = 6;
	cfg.detailSampleDist = detailSampleDist < 0.9f ? 0 : cfg.cs * detailSampleDist;
	cfg.detailSampleMaxError = cfg.ch * 1.f;
	rcVcopy(cfg.bmin, Vec3(0, 0, 0));
	rcVcopy(cfg.bmax, Vec3(map_size, 2.f, map_size));
	rcCalcGridSize(cfg.bmin, cfg.bmax, cfg.cs, &cfg.width, &cfg.height);

	//
	// Step 2. Rasterize input polygon soup.
	//

	// Allocate voxel heightfield where we rasterize our input data to.
	solid = rcAllocHeightfield();
	if(!rcCreateHeightfield(&ctx, *solid, cfg.width, cfg.height, cfg.bmin, cfg.bmax, cfg.cs, cfg.ch))
	{
		ctx.log(RC_LOG_ERROR, "Could not create solid heightfield.");
		return false;
	}

	const int n_tris = 2;
	Vec3 verts[] = {
		Vec3(0,0,map_size),
		Vec3(map_size,0,map_size),
		Vec3(0,0,0),
		Vec3(map_size,0,0)
	};
	const int n_verts = 4;
	int tris[] = {
		0, 1, 2,
		2, 1, 3
	};

	// Allocate array that can hold triangle area types.
	// If you have multiple meshes you need to process, allocate
	// and array which can hold the max number of triangles you need to process.
	triareas.resize(n_tris);

	// Find triangles which are walkable based on their slope and rasterize them.
	// If your input data is multiple meshes, you can transform them here, calculate
	// the are type for each of the meshes and rasterize them.
	memset(triareas.data(), 0, n_tris * sizeof(byte));
	rcMarkWalkableTriangles(&ctx, cfg.walkableSlopeAngle, (float*)verts, n_verts, tris, n_tris, triareas.data());
	if(!rcRasterizeTriangles(&ctx, (float*)verts, triareas.data(), n_tris, *solid, cfg.walkableClimb))
	{
		ctx.log(RC_LOG_ERROR, "Could not rasterize triangles.");
		return false;
	}

	//
	// Step 3. Filter walkables surfaces.
	//

	// Once all geoemtry is rasterized, we do initial pass of filtering to
	// remove unwanted overhangs caused by the conservative rasterization
	// as well as filter spans where the character cannot possibly stand.
	const bool m_filterLowHangingObstacles = true;
	const bool m_filterLedgeSpans = true;
	const bool m_filterWalkableLowHeightSpans = true;
	if(m_filterLowHangingObstacles)
		rcFilterLowHangingWalkableObstacles(&ctx, cfg.walkableClimb, *solid);
	if(m_filterLedgeSpans)
		rcFilterLedgeSpans(&ctx, cfg.walkableHeight, cfg.walkableClimb, *solid);
	if(m_filterWalkableLowHeightSpans)
		rcFilterWalkableLowHeightSpans(&ctx, cfg.walkableHeight, *solid);

	//
	// Step 4. Partition walkable surface to simple regions.
	//

	// Compact the heightfield so that it is faster to handle from now on.
	// This will result more cache coherent data as well as the neighbours
	// between walkable cells will be calculated.
	chf = rcAllocCompactHeightfield();
	if(!rcBuildCompactHeightfield(&ctx, cfg.walkableHeight, cfg.walkableClimb, *solid, *chf))
	{
		ctx.log(RC_LOG_ERROR, "Could not build compact data.");
		return false;
	}

	rcFreeHeightField(solid);
	solid = nullptr;

	// Erode the walkable area by agent radius.
	if(!rcErodeWalkableArea(&ctx, cfg.walkableRadius, *chf))
	{
		ctx.log(RC_LOG_ERROR, "Could not erode.");
		return false;
	}

	// (Optional) Mark areas.
	/// Set area ids TODO FIXME
	//const ConvexVolume* vols = m_geom->getConvexVolumes();
	//for(int i = 0; i < m_geom->getConvexVolumeCount(); ++i)
	//	rcMarkConvexPolyArea(m_ctx, vols[i].verts, vols[i].nverts, vols[i].hmin, vols[i].hmax, (unsigned char)vols[i].area, *m_chf);

	// Partition the heightfield so that we can use simple algorithm later to triangulate the walkable areas.
	// Using Watershed partitioning

	// Prepare for region partitioning, by calculating distance field along the walkable surface.
	if(!rcBuildDistanceField(&ctx, *chf))
	{
		ctx.log(RC_LOG_ERROR, "Could not build distance field.");
		return false;
	}

	// Partition the walkable surface into simple regions without holes.
	if(!rcBuildRegions(&ctx, *chf, 0, cfg.minRegionArea, cfg.mergeRegionArea))
	{
		ctx.log(RC_LOG_ERROR, "Could not build watershed regions.");
		return false;
	}

	//
	// Step 5. Trace and simplify region contours.
	//

	// Create contours.
	cset = rcAllocContourSet();
	if(!rcBuildContours(&ctx, *chf, cfg.maxSimplificationError, cfg.maxEdgeLen, *cset))
	{
		ctx.log(RC_LOG_ERROR, "Could not create contours.");
		return false;
	}

	//
	// Step 6. Build polygons mesh from contours.
	//

	// Build polygon navmesh from the contours.
	pmesh = rcAllocPolyMesh();
	if(!rcBuildPolyMesh(&ctx, *cset, cfg.maxVertsPerPoly, *pmesh))
	{
		ctx.log(RC_LOG_ERROR, "buildNavigation: Could not triangulate contours.");
		return false;
	}

	//
	// Step 7. Create detail mesh which allows to access approximate height on each polygon.
	//

	dmesh = rcAllocPolyMeshDetail();
	if(!rcBuildPolyMeshDetail(&ctx, *pmesh, *chf, cfg.detailSampleDist, cfg.detailSampleMaxError, *dmesh))
	{
		ctx.log(RC_LOG_ERROR, "buildNavigation: Could not build detail mesh.");
		return false;
	}

	rcFreeCompactHeightfield(chf);
	chf = nullptr;
	rcFreeContourSet(cset);
	cset = nullptr;

	// At this point the navigation mesh data is ready, you can access it from pmesh.
	// See duDebugDrawPolyMesh or dtCreateNavMeshData as examples how to access the data.

	//
	// (Optional) Step 8. Create Detour data from Recast poly mesh.
	//

	unsigned char* navData = 0;
	int navDataSize = 0;

	// Update poly flags from areas.
	for(int i = 0; i < pmesh->npolys; ++i)
	{
		if(pmesh->areas[i] == RC_WALKABLE_AREA)
		{
			pmesh->areas[i] = POLYAREA_GROUND;
			pmesh->flags[i] = POLYFLAGS_WALK;
		}
	}


	dtNavMeshCreateParams params = {};
	params.verts = pmesh->verts;
	params.vertCount = pmesh->nverts;
	params.polys = pmesh->polys;
	params.polyAreas = pmesh->areas;
	params.polyFlags = pmesh->flags;
	params.polyCount = pmesh->npolys;
	params.nvp = pmesh->nvp;
	params.detailMeshes = dmesh->meshes;
	params.detailVerts = dmesh->verts;
	params.detailVertsCount = dmesh->nverts;
	params.detailTris = dmesh->tris;
	params.detailTriCount = dmesh->ntris;
	/*params.offMeshConVerts = m_geom->getOffMeshConnectionVerts();
	params.offMeshConRad = m_geom->getOffMeshConnectionRads();
	params.offMeshConDir = m_geom->getOffMeshConnectionDirs();
	params.offMeshConAreas = m_geom->getOffMeshConnectionAreas();
	params.offMeshConFlags = m_geom->getOffMeshConnectionFlags();
	params.offMeshConUserID = m_geom->getOffMeshConnectionId();
	params.offMeshConCount = m_geom->getOffMeshConnectionCount();*/
	params.walkableHeight = agent_height;
	params.walkableRadius = agent_radius;
	params.walkableClimb = agent_climb;
	rcVcopy(params.bmin, pmesh->bmin);
	rcVcopy(params.bmax, pmesh->bmax);
	params.cs = cfg.cs;
	params.ch = cfg.ch;
	params.buildBvTree = true;

	if(!dtCreateNavMeshData(&params, &navData, &navDataSize))
	{
		ctx.log(RC_LOG_ERROR, "Could not build Detour navmesh.");
		return false;
	}

	navmesh = dtAllocNavMesh();
	dtStatus status = navmesh->init(navData, navDataSize, DT_TILE_FREE_DATA);
	if(dtStatusFailed(status))
	{
		dtFree(navData);
		ctx.log(RC_LOG_ERROR, "Could not init Detour navmesh");
		return false;
	}

	status = nav_query->init(navmesh, 2048);
	if(dtStatusFailed(status))
	{
		ctx.log(RC_LOG_ERROR, "Could not init Detour navmesh query");
		return false;
	}

	Info("Finished building navmesh. Total time %g sec.", t.Tick());

	return true;
}

//void Navmesh::EndRegion()
//{


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
//}

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

/*float my_dt;

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
*/

void Navmesh::FindPath(const Vec3& from, const Vec3& to)
{
	start_pos = from;
	end_pos = to;
	have_path = false;

	const float ext[] = { 2.f, 4.f, 2.f };

	dtPolyRef start_ref, end_ref;
	nav_query->findNearestPoly(from, ext, &filter, &start_ref, nullptr);
	nav_query->findNearestPoly(to, ext, &filter, &end_ref, nullptr);

	if(start_ref != 0 && end_ref != 0)
	{
		nav_query->findPath(start_ref, end_ref, from, to, &filter, path, &path_length, MAX_PATH);
		if(path_length != 0)
			have_path = true;
	}
}

void Navmesh::Draw(DebugDrawer* debug_drawer)
{
	if(!navmesh)
		return;

	Vec3 tri[3];

	debug_drawer->SetColor(Color(0, 128, 255, 128));

	for(int tile_i = 0, count = navmesh->getMaxTiles(); tile_i < count; ++tile_i)
	{
		const dtMeshTile* tile = navmesh->getTile(tile_i);
		if(!tile->header)
			continue;

		for(int i = 0; i < tile->header->polyCount; ++i)
		{
			const dtPolyDetail& pd = tile->detailMeshes[i];
			for(int j = 0; j < pd.triCount; ++j)
			{
				const unsigned char* t = &tile->detailTris[(pd.triBase + j) * 4];
				for(int k = 0; k < 3; ++k)
				{
					if(t[k] < p->vertCount)
						tri[k] = &tile->verts[p->verts[t[k]] * 3];
					else
						tri[k] = &tile->detailVerts[(pd->vertBase + t[k] - p->vertCount) * 3];
				}

				debug_drawer->DrawTriangle(tri);
			}
		}
	}

	if(have_path)
	{
		debug_drawer->SetColor(Color(0, 0, 255));
		debug_drawer->DrawSphere(start_pos + Vec3(0, 0.1f, 0), 0.3f);

		debug_drawer->SetColor(Color(255, 0, 0));
		debug_drawer->DrawSphere(end_pos + Vec3(0, 0.1f, 0), 0.3f);
	}
}
