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

	have_path = false;
	start_set = false;
	end_set = false;
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
	start_set = false;
	end_set = false;

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

void Navmesh::SetPos(const Vec3& pos, bool start)
{
	if(start)
	{
		start_pos = pos;
		start_set = true;
	}
	else
	{
		end_pos = pos;
		end_set = true;
	}
	if(start_set && end_set)
		FindPath(start_pos, end_pos);
}

void Navmesh::FindPath(const Vec3& from, const Vec3& to)
{
	have_path = false;

	const float ext[] = { 2.f, 4.f, 2.f };

	nav_query->findNearestPoly(from, ext, &filter, &start_ref, nullptr);
	nav_query->findNearestPoly(to, ext, &filter, &end_ref, nullptr);

	if(start_ref != 0 && end_ref != 0)
	{
		nav_query->findPath(start_ref, end_ref, from, to, &filter, path, &path_length, MAX_PATH);
		if(path_length == 0)
			return;
		
		have_path = true;
		smooth_path.clear();

		dtPolyRef polys[MAX_PATH];
		memcpy(polys, path, sizeof(dtPolyRef)*path_length);
		int npolys = path_length;

		Vec3 iterPos, targetPos;
		nav_query->closestPointOnPoly(start_ref, start_pos, iterPos, nullptr);
		nav_query->closestPointOnPoly(polys[npolys - 1], end_pos, targetPos, nullptr);

		static const float STEP_SIZE = 0.5f;
		static const float SLOP = 0.01f;

		smooth_path.push_back(iterPos);

		// Move towards target a small advancement at a time until target reached or
		// when ran out of memory to store the path.
		while(npolys && smooth_path.size() < MAX_SMOOTH)
		{
			// Find location to steer towards.
			float steerPos[3];
			unsigned char steerPosFlag;
			dtPolyRef steerPosRef;

			if(!getSteerTarget(m_navQuery, iterPos, targetPos, SLOP,
				polys, npolys, steerPos, steerPosFlag, steerPosRef))
				break;

			bool endOfPath = (steerPosFlag & DT_STRAIGHTPATH_END) ? true : false;
			bool offMeshConnection = (steerPosFlag & DT_STRAIGHTPATH_OFFMESH_CONNECTION) ? true : false;

			// Find movement delta.
			float delta[3], len;
			dtVsub(delta, steerPos, iterPos);
			len = dtMathSqrtf(dtVdot(delta, delta));
			// If the steer target is end of path or off-mesh link, do not move past the location.
			if((endOfPath || offMeshConnection) && len < STEP_SIZE)
				len = 1;
			else
				len = STEP_SIZE / len;
			float moveTgt[3];
			dtVmad(moveTgt, iterPos, delta, len);

			// Move
			float result[3];
			dtPolyRef visited[16];
			int nvisited = 0;
			m_navQuery->moveAlongSurface(polys[0], iterPos, moveTgt, &m_filter,
				result, visited, &nvisited, 16);

			npolys = fixupCorridor(polys, npolys, MAX_POLYS, visited, nvisited);
			npolys = fixupShortcuts(polys, npolys, m_navQuery);

			float h = 0;
			m_navQuery->getPolyHeight(polys[0], result, &h);
			result[1] = h;
			dtVcopy(iterPos, result);

			// Handle end of path and off-mesh links when close enough.
			if(endOfPath && inRange(iterPos, steerPos, SLOP, 1.0f))
			{
				// Reached end of path.
				dtVcopy(iterPos, targetPos);
				if(m_nsmoothPath < MAX_SMOOTH)
				{
					dtVcopy(&m_smoothPath[m_nsmoothPath * 3], iterPos);
					m_nsmoothPath++;
				}
				break;
			}
			else if(offMeshConnection && inRange(iterPos, steerPos, SLOP, 1.0f))
			{
				// Reached off-mesh connection.
				float startPos[3], endPos[3];

				// Advance the path up to and over the off-mesh connection.
				dtPolyRef prevRef = 0, polyRef = polys[0];
				int npos = 0;
				while(npos < npolys && polyRef != steerPosRef)
				{
					prevRef = polyRef;
					polyRef = polys[npos];
					npos++;
				}
				for(int i = npos; i < npolys; ++i)
					polys[i - npos] = polys[i];
				npolys -= npos;

				// Handle the connection.
				dtStatus status = m_navMesh->getOffMeshConnectionPolyEndPoints(prevRef, polyRef, startPos, endPos);
				if(dtStatusSucceed(status))
				{
					if(m_nsmoothPath < MAX_SMOOTH)
					{
						dtVcopy(&m_smoothPath[m_nsmoothPath * 3], startPos);
						m_nsmoothPath++;
						// Hack to make the dotted path not visible during off-mesh connection.
						if(m_nsmoothPath & 1)
						{
							dtVcopy(&m_smoothPath[m_nsmoothPath * 3], startPos);
							m_nsmoothPath++;
						}
					}
					// Move position at the other side of the off-mesh link.
					dtVcopy(iterPos, endPos);
					float eh = 0.0f;
					m_navQuery->getPolyHeight(polys[0], iterPos, &eh);
					iterPos[1] = eh;
				}
			}

			// Store results.
			if(m_nsmoothPath < MAX_SMOOTH)
			{
				dtVcopy(&m_smoothPath[m_nsmoothPath * 3], iterPos);
				m_nsmoothPath++;
			}
		}

		// FIXME
		Info("Path found, length:%u, smooth:%u", path_length, smooth_path.size());
	}
}

void Navmesh::Draw(DebugDrawer* debug_drawer)
{
	if(!navmesh)
		return;

	// !!! depth mask = false

	DrawNavmesh(debug_drawer, *navmesh, *nav_query);

	if(start_set)
	{
		debug_drawer->SetColor(Color(0, 0, 255, 200));
		debug_drawer->DrawSphere(start_pos + Vec3(0, 0.1f, 0), 0.3f);
	}

	if(end_set)
	{
		debug_drawer->SetColor(Color(255, 0, 0, 200));
		debug_drawer->DrawSphere(end_pos + Vec3(0, 0.1f, 0), 0.3f);
	}

	if(have_path)
		DrawPath(debug_drawer, *navmesh);
}

// duDebugDrawNavMeshWithClosedList
void Navmesh::DrawNavmesh(DebugDrawer* debug_drawer, const dtNavMesh& mesh, const dtNavMeshQuery& query)
{
	for(int i = 0, count = mesh.getMaxTiles(); i < count; ++i)
	{
		const dtMeshTile* tile = mesh.getTile(i);
		if(tile->header)
			DrawMeshTile(debug_drawer, mesh, query, tile);
	}
}

int bit(int a, int b)
{
	return (a & (1 << b)) >> b;
}

Color IntToCol(int i, int a)
{
	int	r = bit(i, 1) + bit(i, 3) * 2 + 1;
	int	g = bit(i, 2) + bit(i, 4) * 2 + 1;
	int	b = bit(i, 0) + bit(i, 5) * 2 + 1;
	return Color(r * 63, g * 63, b * 63, a);
}

// drawMeshTile
void Navmesh::DrawMeshTile(DebugDrawer* debug_drawer, const dtNavMesh& mesh, const dtNavMeshQuery& query, const dtMeshTile* tile)
{
	dtPolyRef base = mesh.getPolyRefBase(tile);

	for(int i = 0; i < tile->header->polyCount; ++i)
	{
		const dtPoly* p = &tile->polys[i];
		if(p->getType() == DT_POLYTYPE_OFFMESH_CONNECTION)	// Skip off-mesh links.
			continue;

		const dtPolyDetail* pd = &tile->detailMeshes[i];

		Color col;
		if(query.isInClosedList(base | (dtPolyRef)i))
			col = Color(255, 196, 0, 64);
		else
			col = Color(0, 192, 255, 64);
		debug_drawer->SetColor(col);

		for(int j = 0; j < pd->triCount; ++j)
		{
			const unsigned char* t = &tile->detailTris[(pd->triBase + j) * 4];
			Vec3 tri[3];
			for(int k = 0; k < 3; ++k)
			{
				if(t[k] < p->vertCount)
					tri[k] = Vec3(&tile->verts[p->verts[t[k]] * 3]);
				else
					tri[k] = Vec3(&tile->detailVerts[(pd->vertBase + t[k] - p->vertCount) * 3]);
			}
			debug_drawer->DrawTriangle(tri);
		}
	}

	DrawPolyBoundaries(debug_drawer, tile, Color(0, 48, 64, 64), 0.05f, true);
	DrawPolyBoundaries(debug_drawer, tile, Color(0, 48, 64, 250), 0.1f, false);
}

static float distancePtLine2d(const float* pt, const float* p, const float* q)
{
	float pqx = q[0] - p[0];
	float pqz = q[2] - p[2];
	float dx = pt[0] - p[0];
	float dz = pt[2] - p[2];
	float d = pqx * pqx + pqz * pqz;
	float t = pqx * dx + pqz * dz;
	if(d != 0)
		t /= d;
	dx = p[0] + t * pqx - pt[0];
	dz = p[2] + t * pqz - pt[2];
	return dx * dx + dz * dz;
}

// drawPolyBoundaries
void Navmesh::DrawPolyBoundaries(DebugDrawer* debug_drawer, const dtMeshTile* tile, Color color, float line_width, bool inner)
{
	static const float thr = 0.01f*0.01f;
	debug_drawer->SetColor(color);

	for(int i = 0; i < tile->header->polyCount; ++i)
	{
		const dtPoly* p = &tile->polys[i];

		if(p->getType() == DT_POLYTYPE_OFFMESH_CONNECTION)
			continue;

		const dtPolyDetail* pd = &tile->detailMeshes[i];

		for(int j = 0, nj = (int)p->vertCount; j < nj; ++j)
		{
			if(inner)
			{
				if(p->neis[j] == 0)
					continue;
			}
			else
			{
				if(p->neis[j] != 0)
					continue;
			}

			const float* v0 = &tile->verts[p->verts[j] * 3];
			const float* v1 = &tile->verts[p->verts[(j + 1) % nj] * 3];

			// Draw detail mesh edges which align with the actual poly edge.
			// This is really slow.
			for(int k = 0; k < pd->triCount; ++k)
			{
				const unsigned char* t = &tile->detailTris[(pd->triBase + k) * 4];
				const float* tv[3];
				for(int m = 0; m < 3; ++m)
				{
					if(t[m] < p->vertCount)
						tv[m] = &tile->verts[p->verts[t[m]] * 3];
					else
						tv[m] = &tile->detailVerts[(pd->vertBase + (t[m] - p->vertCount)) * 3];
				}
				for(int m = 0, n = 2; m < 3; n = m++)
				{
					if(((t[3] >> (n * 2)) & 0x3) == 0)
						continue;	// Skip inner detail edges.
					if(distancePtLine2d(tv[n], v0, v1) < thr && distancePtLine2d(tv[m], v0, v1) < thr)
						debug_drawer->DrawLine(Vec3(tv[n]), Vec3(tv[m]), line_width);
				}
			}
		}
	}
}

void Navmesh::DrawPath(DebugDrawer* debug_drawer, const dtNavMesh& mesh)
{
	DrawPoly(debug_drawer, mesh, start_ref, Color(128, 25, 0, 64));
	DrawPoly(debug_drawer, mesh, end_ref, Color(51, 102, 0, 64));

	if(path_length)
	{
		for(int i = 0; i < path_length; ++i)
		{
			dtPolyRef poly = path[i];
			if(poly == start_ref || poly == end_ref)
				continue;
			DrawPoly(debug_drawer, mesh, poly, Color(0, 0, 0, 64));
		}
	}

	if(!smooth_path.empty())
	{
		debug_drawer->SetColor(Color(0, 0, 0, 220));
		Vec3 pts[2];
		int index = 0;
		for(const Vec3& pt : smooth_path)
		{
			pts[index] = pt;
			++index;
			if(index == 2)
			{
				index = 0;
				debug_drawer->DrawLine(pts[0], pts[1], 0.1f);
			}
		}
	}
}

// duDebugDrawNavMeshPoly
void Navmesh::DrawPoly(DebugDrawer* debug_drawer, const dtNavMesh& mesh, dtPolyRef ref, Color col)
{
	const dtMeshTile* tile = 0;
	const dtPoly* poly = 0;
	if(dtStatusFailed(mesh.getTileAndPolyByRef(ref, &tile, &poly)))
		return;

	const unsigned int ip = (unsigned int)(poly - tile->polys);

	assert(poly->getType() != DT_POLYTYPE_OFFMESH_CONNECTION); // removed code
	
	const dtPolyDetail* pd = &tile->detailMeshes[ip];
	debug_drawer->SetColor(col);

	for(int i = 0; i < pd->triCount; ++i)
	{
		const unsigned char* t = &tile->detailTris[(pd->triBase + i) * 4];
		Vec3 tri[3];
		for(int j = 0; j < 3; ++j)
		{
			if(t[j] < poly->vertCount)
				tri[j] = Vec3(&tile->verts[poly->verts[t[j]] * 3]);
			else
				tri[j] = Vec3(&tile->detailVerts[(pd->vertBase + t[j] - poly->vertCount) * 3]);
		}
		debug_drawer->DrawTriangle(tri);
	}
}
