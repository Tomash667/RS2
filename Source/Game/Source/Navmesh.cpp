#include "GameCore.h"
#include "Navmesh.h"
#include <DebugDrawer.h>
#include <Recast.h>
#include <DetourNavMesh.h>
#include <DetourNavMeshQuery.h>
#include <DetourNavMeshBuilder.h>
#include "Unit.h"

const float CELL_SIZE = 0.25f;
const float CELL_HEIGHT = 0.25f;
const float AGENT_RADIUS = Unit::radius;
const float AGENT_HEIGHT = Unit::height;
const float AGENT_CLIMB = 0.25f;
const int NAV_QUERY_MAX_NODES = 2048;

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
static BuildContext ctx;

Navmesh::Navmesh() : solid(nullptr), chf(nullptr), cset(nullptr), pmesh(nullptr), dmesh(nullptr), navmesh(nullptr)
{
	nav_query = dtAllocNavMeshQuery();

	filter = new dtQueryFilter;
	filter->setIncludeFlags(POLYFLAGS_WALK);
	filter->setExcludeFlags(0);

	test_path.ok = false;
}

Navmesh::~Navmesh()
{
	Reset();
	dtFreeNavMeshQuery(nav_query);
	delete filter;
}

void Navmesh::Reset()
{
	test_path.ok = false;

	Cleanup();
	dtFreeNavMesh(navmesh);
	navmesh = nullptr;
}

void Navmesh::Cleanup()
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
}

bool Navmesh::PrepareTiles(float tile_size, uint tiles)
{
	assert(tile_size >= 1.f && tiles >= 1u);

	Info("Building tiled mesh...");

	Reset();
	navmesh = dtAllocNavMesh();

	dtNavMeshParams params;
	rcVcopy(params.orig, Vec3(0, 0, 0));
	params.tileHeight = tile_size;
	params.tileWidth = params.tileHeight;
	params.maxTiles = tiles * tiles;
	params.maxPolys = 512;

	dtStatus status = navmesh->init(&params);
	if(dtStatusFailed(status))
	{
		Error("Failed to init tiled navmesh.");
		return false;
	}

	status = nav_query->init(navmesh, NAV_QUERY_MAX_NODES);
	if(dtStatusFailed(status))
	{
		Error("Failed to init tiled navmesh query.");
		return false;
	}

	is_tiled = true;
	this->tile_size = tile_size;
	this->tiles = tiles;

	return true;
}

bool Navmesh::Build(const NavmeshGeometry& geom)
{
	Reset();
	is_tiled = false;

	byte* navData;
	int navDataSize;
	if(!BuildTileMesh(Int2(0, 0), geom, navData, navDataSize))
		return false;

	navmesh = dtAllocNavMesh();
	dtStatus status = navmesh->init(navData, navDataSize, DT_TILE_FREE_DATA);
	if(dtStatusFailed(status))
	{
		dtFree(navData);
		ctx.log(RC_LOG_ERROR, "Could not init Detour navmesh");
		return false;
	}

	status = nav_query->init(navmesh, NAV_QUERY_MAX_NODES);
	if(dtStatusFailed(status))
	{
		ctx.log(RC_LOG_ERROR, "Could not init Detour navmesh query");
		return false;
	}

	return true;
}

bool Navmesh::BuildTile(const Int2& tile, const NavmeshGeometry& geom)
{
	assert(navmesh && is_tiled);

	byte* data;
	int data_size;

	if(!BuildTileMesh(tile, geom, data, data_size))
		return false;

	dtStatus status = navmesh->addTile(data, data_size, DT_TILE_FREE_DATA, 0, nullptr);
	if(dtStatusFailed(status))
	{
		dtFree(data);
		return false;
	}

	return true;
}

bool Navmesh::BuildTileMesh(const Int2& tile, const NavmeshGeometry& geom, byte*& data, int& data_size)
{
	Cleanup();

	//
	// Step 1. Initialize build config.
	//

	// Init configuration
	const float detailSampleDist = 6.f;
	rcConfig cfg = {};
	cfg.cs = CELL_SIZE;
	cfg.ch = CELL_HEIGHT;
	cfg.walkableSlopeAngle = 45.f;
	cfg.walkableHeight = (int)ceilf(AGENT_HEIGHT / cfg.ch);
	cfg.walkableClimb = (int)ceilf(AGENT_CLIMB / cfg.ch); // in cells
	cfg.walkableRadius = (int)ceilf(AGENT_RADIUS / cfg.cs);
	cfg.maxEdgeLen = 80;
	cfg.maxSimplificationError = 1.3f;
	cfg.minRegionArea = rcSqr(10);
	cfg.mergeRegionArea = rcSqr(20);
	cfg.maxVertsPerPoly = 6;
	cfg.detailSampleDist = detailSampleDist < 0.9f ? 0 : cfg.cs * detailSampleDist;
	cfg.detailSampleMaxError = cfg.ch * 1.f;
	rcVcopy(cfg.bmin, geom.bounds.v1);
	rcVcopy(cfg.bmax, geom.bounds.v2);
	if(is_tiled)
	{
		cfg.tileSize = (int)(tile_size / cfg.cs);
		cfg.borderSize = cfg.walkableRadius + 3;
		cfg.width = cfg.tileSize + cfg.borderSize * 2;
		cfg.height = cfg.width;
	}
	else
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

	// Allocate array that can hold triangle area types.
	// If you have multiple meshes you need to process, allocate
	// and array which can hold the max number of triangles you need to process.
	triareas.resize(geom.tri_count);

	// Find triangles which are walkable based on their slope and rasterize them.
	// If your input data is multiple meshes, you can transform them here, calculate
	// the are type for each of the meshes and rasterize them.
	memset(triareas.data(), 0, geom.tri_count * sizeof(byte));
	rcMarkWalkableTriangles(&ctx, cfg.walkableSlopeAngle, (float*)geom.verts, geom.vert_count, geom.tris, geom.tri_count, triareas.data());
	if(!rcRasterizeTriangles(&ctx, (float*)geom.verts, geom.vert_count, geom.tris, triareas.data(), geom.tri_count, *solid, cfg.walkableClimb))
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

	/// Set area ids TODO
	// (Optional) Mark areas.
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
	if(!rcBuildRegions(&ctx, *chf, cfg.borderSize, cfg.minRegionArea, cfg.mergeRegionArea))
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

	if(cset->nconts == 0)
		return false;

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

	byte* navData = 0;
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
	params.walkableHeight = AGENT_HEIGHT;
	params.walkableRadius = AGENT_RADIUS;
	params.walkableClimb = AGENT_CLIMB;
	rcVcopy(params.bmin, pmesh->bmin);
	rcVcopy(params.bmax, pmesh->bmax);
	params.cs = cfg.cs;
	params.ch = cfg.ch;
	params.buildBvTree = true;
	if(is_tiled)
	{
		params.tileX = tile.x;
		params.tileY = tile.y;
	}

	if(!dtCreateNavMeshData(&params, &navData, &navDataSize))
	{
		ctx.log(RC_LOG_ERROR, "Could not build Detour navmesh.");
		return false;
	}

	data = navData;
	data_size = navDataSize;
	return true;
}

Box2d Navmesh::GetBoxForTile(const Int2& tile)
{
	assert(tile.x >= 0 && tile.y >= 0 && tile.x < (int)tiles && tile.y < (int)tiles);
	const int walkableRadius = (int)(AGENT_RADIUS / CELL_SIZE);
	const int borderSize = walkableRadius + 3;
	const float border = borderSize * CELL_SIZE;
	return Box2d(tile_size * tile.x - border, tile_size * tile.y - border,
		tile_size * (tile.x + 1) + border, tile_size * (tile.y + 1) + border);
}

dtPolyRef Navmesh::GetPolyRef(const Vec3& pos)
{
	static const float ext[] = { 2.f, 4.f, 2.f };
	dtPolyRef ref;
	nav_query->findNearestPoly(pos, ext, filter, &ref, nullptr);
	return ref;
}

bool Navmesh::FindPath(const Vec3& from, const Vec3& to, vector<Vec3>& out_path)
{
	const float ext[] = { 2.f, 4.f, 2.f };

	dtPolyRef start_ref, end_ref;
	nav_query->findNearestPoly(from, ext, filter, &start_ref, nullptr);
	nav_query->findNearestPoly(to, ext, filter, &end_ref, nullptr);

	if(start_ref == 0 || end_ref == 0)
		return false;

	nav_query->findPath(start_ref, end_ref, from, to, filter, tmp_path, &tmp_path_length, MAX_POLYS);
	if(tmp_path_length == 0)
		return false;

	FindStraightPath(end_ref, from, to, out_path);
	return true;
}

bool Navmesh::FindTestPath(const Vec3& from, const Vec3& to, bool smooth)
{
	test_path.ok = false;
	test_path.smooth = smooth;
	test_path.start_pos = from;
	test_path.end_pos = to;

	const float ext[] = { 2.f, 4.f, 2.f };

	nav_query->findNearestPoly(from, ext, filter, &test_path.start_ref, nullptr);
	nav_query->findNearestPoly(to, ext, filter, &test_path.end_ref, nullptr);

	if(test_path.start_ref == 0 || test_path.end_ref == 0)
		return false;

	nav_query->findPath(test_path.start_ref, test_path.end_ref, from, to, filter, tmp_path, &tmp_path_length, MAX_POLYS);
	if(tmp_path_length == 0)
		return false;

	test_path.ok = true;
	if(!smooth)
	{
		FindStraightPath(test_path.end_ref, from, test_path.end_pos, test_path.path);
		Info("Path found, length:%d, straight:%d", tmp_path_length, test_path.path.size());
	}
	else
	{
		SmoothPath(test_path.start_ref, from, to, test_path.path);
		Info("Path found, length:%d, smooth:%d", tmp_path_length, test_path.path.size());
	}

	return true;
}

void Navmesh::FindStraightPath(dtPolyRef end_ref, const Vec3& start_pos, const Vec3& end_pos, vector<Vec3>& out_path)
{
	// In case of partial path, make sure the end point is clamped to the last polygon.
	Vec3 epos = end_pos;
	if(tmp_path[tmp_path_length - 1] != end_ref)
		nav_query->closestPointOnPoly(tmp_path[tmp_path_length - 1], end_pos, epos, nullptr);

	int length = 0;
	nav_query->findStraightPath(start_pos, epos, tmp_path, tmp_path_length, (float*)tmp_straight_path, nullptr,
		nullptr, &length, MAX_POLYS, 0);

	if(length >= 2)
	{
		out_path.resize(length);
		memcpy(out_path.data(), tmp_straight_path, sizeof(Vec3) * length);
	}
	else
	{
		// fix for path starting & ending in same 'mini' tile
		out_path.resize(2);
		out_path[0] = start_pos;
		out_path[1] = end_pos;
	}
}

inline bool inRange(const Vec3& v1, const Vec3& v2, const float r, const float h)
{
	const float dx = v2.x - v1.x;
	const float dy = v2.y - v1.y;
	const float dz = v2.z - v1.z;
	return (dx*dx + dz * dz) < r*r && fabsf(dy) < h;
}

static int fixupCorridor(dtPolyRef* path, const int npath, const int maxPath,
	const dtPolyRef* visited, const int nvisited)
{
	int furthestPath = -1;
	int furthestVisited = -1;

	// Find furthest common polygon.
	for(int i = npath - 1; i >= 0; --i)
	{
		bool found = false;
		for(int j = nvisited - 1; j >= 0; --j)
		{
			if(path[i] == visited[j])
			{
				furthestPath = i;
				furthestVisited = j;
				found = true;
			}
		}
		if(found)
			break;
	}

	// If no intersection found just return current path. 
	if(furthestPath == -1 || furthestVisited == -1)
		return npath;

	// Concatenate paths.	

	// Adjust beginning of the buffer to include the visited.
	const int req = nvisited - furthestVisited;
	const int orig = rcMin(furthestPath + 1, npath);
	int size = rcMax(0, npath - orig);
	if(req + size > maxPath)
		size = maxPath - req;
	if(size)
		memmove(path + req, path + orig, size * sizeof(dtPolyRef));

	// Store visited
	for(int i = 0; i < req; ++i)
		path[i] = visited[(nvisited - 1) - i];

	return req + size;
}

// This function checks if the path has a small U-turn, that is,
// a polygon further in the path is adjacent to the first polygon
// in the path. If that happens, a shortcut is taken.
// This can happen if the target (T) location is at tile boundary,
// and we're (S) approaching it parallel to the tile edge.
// The choice at the vertex can be arbitrary, 
//  +---+---+
//  |:::|:::|
//  +-S-+-T-+
//  |:::|   | <-- the step can end up in here, resulting U-turn path.
//  +---+---+
static int fixupShortcuts(dtPolyRef* path, int npath, dtNavMeshQuery* navQuery)
{
	if(npath < 3)
		return npath;

	// Get connected polygons
	static const int maxNeis = 16;
	dtPolyRef neis[maxNeis];
	int nneis = 0;

	const dtMeshTile* tile = 0;
	const dtPoly* poly = 0;
	if(dtStatusFailed(navQuery->getAttachedNavMesh()->getTileAndPolyByRef(path[0], &tile, &poly)))
		return npath;

	for(unsigned int k = poly->firstLink; k != DT_NULL_LINK; k = tile->links[k].next)
	{
		const dtLink* link = &tile->links[k];
		if(link->ref != 0)
		{
			if(nneis < maxNeis)
				neis[nneis++] = link->ref;
		}
	}

	// If any of the neighbour polygons is within the next few polygons
	// in the path, short cut to that polygon directly.
	static const int maxLookAhead = 6;
	int cut = 0;
	for(int i = min(maxLookAhead, npath) - 1; i > 1 && cut == 0; i--)
	{
		for(int j = 0; j < nneis; j++)
		{
			if(path[i] == neis[j])
			{
				cut = i;
				break;
			}
		}
	}
	if(cut > 1)
	{
		int offset = cut - 1;
		npath -= offset;
		for(int i = 1; i < npath; i++)
			path[i] = path[i + offset];
	}

	return npath;
}

static bool getSteerTarget(const dtNavMeshQuery& query, const Vec3& startPos, const Vec3& endPos, const float minTargetDist,
	const dtPolyRef* path, const int pathSize, Vec3& steerPos, unsigned char& steerPosFlag, dtPolyRef& steerPosRef)
{
	// Find steer target.
	static const int MAX_STEER_POINTS = 3;
	Vec3 steerPath[MAX_STEER_POINTS];
	unsigned char steerPathFlags[MAX_STEER_POINTS];
	dtPolyRef steerPathPolys[MAX_STEER_POINTS];
	int nsteerPath = 0;
	query.findStraightPath(startPos, endPos, path, pathSize,
		(float*)steerPath, steerPathFlags, steerPathPolys, &nsteerPath, MAX_STEER_POINTS);
	if(!nsteerPath)
		return false;

	// Find vertex far enough to steer to.
	int ns = 0;
	while(ns < nsteerPath)
	{
		// Stop at Off-Mesh link or when point is further than slop away.
		if((steerPathFlags[ns] & DT_STRAIGHTPATH_OFFMESH_CONNECTION) ||
			!inRange(steerPath[ns], startPos, minTargetDist, 1000.0f))
			break;
		ns++;
	}
	// Failed to find good point to steer to.
	if(ns >= nsteerPath)
		return false;

	steerPos = steerPath[ns];
	steerPos.y = startPos.y;
	steerPosFlag = steerPathFlags[ns];
	steerPosRef = steerPathPolys[ns];

	return true;
}

void Navmesh::SmoothPath(dtPolyRef start_ref, const Vec3& start_pos, const Vec3& end_pos, vector<Vec3>& out_path)
{
	test_path.path.clear();

	dtPolyRef polys[MAX_POLYS];
	memcpy(polys, tmp_path, sizeof(dtPolyRef)*tmp_path_length);
	int npolys = tmp_path_length;

	Vec3 iterPos, targetPos;
	nav_query->closestPointOnPoly(start_ref, start_pos, iterPos, nullptr);
	nav_query->closestPointOnPoly(polys[npolys - 1], end_pos, targetPos, nullptr);

	static const float STEP_SIZE = 0.5f;
	static const float SLOP = 0.01f;

	test_path.path.push_back(iterPos);

	// Move towards target a small advancement at a time until target reached or
	// when ran out of memory to store the path.
	while(npolys && test_path.path.size() < MAX_SMOOTH)
	{
		// Find location to steer towards.
		Vec3 steerPos;
		unsigned char steerPosFlag;
		dtPolyRef steerPosRef;

		if(!getSteerTarget(*nav_query, iterPos, targetPos, SLOP, polys, npolys, steerPos, steerPosFlag, steerPosRef))
			break;

		bool endOfPath = (steerPosFlag & DT_STRAIGHTPATH_END) ? true : false;
		bool offMeshConnection = (steerPosFlag & DT_STRAIGHTPATH_OFFMESH_CONNECTION) ? true : false;

		// Find movement delta.
		Vec3 delta = steerPos - iterPos;
		float len = sqrtf(delta.Dot2d());
		// If the steer target is end of path or off-mesh link, do not move past the location.
		if((endOfPath || offMeshConnection) && len < STEP_SIZE)
			len = 1;
		else
			len = STEP_SIZE / len;
		Vec3 moveTgt = iterPos + delta * len;

		// Move
		Vec3 result;
		dtPolyRef visited[16];
		int nvisited = 0;
		nav_query->moveAlongSurface(polys[0], iterPos, moveTgt, filter, result, visited, &nvisited, 16);

		npolys = fixupCorridor(polys, npolys, MAX_POLYS, visited, nvisited);
		npolys = fixupShortcuts(polys, npolys, nav_query);

		float h = 0;
		nav_query->getPolyHeight(polys[0], result, &h);
		result[1] = h;
		iterPos = result;

		// Handle end of path and off-mesh links when close enough.
		if(endOfPath && inRange(iterPos, steerPos, SLOP, 1.0f))
		{
			// Reached end of path.
			iterPos = targetPos;
			if(test_path.path.size() < MAX_SMOOTH)
				test_path.path.push_back(iterPos);
			break;
		}
		else if(offMeshConnection && inRange(iterPos, steerPos, SLOP, 1.0f))
		{
			// Reached off-mesh connection.
			Vec3 startPos, endPos;

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
			dtStatus status = navmesh->getOffMeshConnectionPolyEndPoints(prevRef, polyRef, startPos, endPos);
			if(dtStatusSucceed(status))
			{
				if(test_path.path.size() < MAX_SMOOTH)
				{
					test_path.path.push_back(startPos);
					// Hack to make the dotted path not visible during off-mesh connection.
					if(test_path.path.size() & 1)
						test_path.path.push_back(startPos);
				}
				// Move position at the other side of the off-mesh link.
				iterPos = endPos;
				float eh = 0.0f;
				nav_query->getPolyHeight(polys[0], iterPos, &eh);
				iterPos[1] = eh;
			}
		}

		// Store results.
		if(test_path.path.size() < MAX_SMOOTH)
			test_path.path.push_back(iterPos);
	}
}

void Navmesh::Draw(DebugDrawer* debug_drawer)
{
	if(!navmesh)
		return;

	debug_drawer->BeginBatch();
	DrawNavmesh(debug_drawer, *navmesh);
	debug_drawer->EndBatch();

	if(test_path.ok)
		DrawPath(debug_drawer, test_path.path, test_path.smooth);
}

// duDebugDrawNavMeshWithClosedList
void Navmesh::DrawNavmesh(DebugDrawer* debug_drawer, const dtNavMesh& mesh)
{
	for(int i = 0, count = mesh.getMaxTiles(); i < count; ++i)
	{
		const dtMeshTile* tile = mesh.getTile(i);
		if(tile->header)
			DrawMeshTile(debug_drawer, mesh, tile);
	}
}

// drawMeshTile (removed using query - can't use it for selected path)
void Navmesh::DrawMeshTile(DebugDrawer* debug_drawer, const dtNavMesh& mesh, const dtMeshTile* tile)
{
	debug_drawer->SetColor(Color(0, 192, 255, 64));

	for(int i = 0; i < tile->header->polyCount; ++i)
	{
		const dtPoly* p = &tile->polys[i];
		if(p->getType() == DT_POLYTYPE_OFFMESH_CONNECTION)	// Skip off-mesh links.
			continue;

		const dtPolyDetail* pd = &tile->detailMeshes[i];

		for(int j = 0; j < pd->triCount; ++j)
		{
			const unsigned char* t = &tile->detailTris[(pd->triBase + j) * 4];
			for(int k = 0; k < 3; ++k)
			{
				if(t[k] < p->vertCount)
					debug_drawer->AddVertex(Vec3(&tile->verts[p->verts[t[k]] * 3]));
				else
					debug_drawer->AddVertex(Vec3(&tile->detailVerts[(pd->vertBase + t[k] - p->vertCount) * 3]));
			}
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
						continue; // Skip inner detail edges.
					if(distancePtLine2d(tv[n], v0, v1) < thr && distancePtLine2d(tv[m], v0, v1) < thr)
						debug_drawer->AddLine(Vec3(tv[n]), Vec3(tv[m]), line_width);
				}
			}
		}
	}
}

/*void Navmesh::DrawPathPoly(DebugDrawer* debug_drawer, const dtNavMesh& mesh)
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
}*/

void Navmesh::DrawPath(DebugDrawer* debug_drawer, const vector<Vec3>& path, bool smooth)
{
	if(path.size() < 2u)
		return;

	debug_drawer->SetColor(Color(0, 0, 255, 200));
	debug_drawer->DrawSphere(path.front() + Vec3(0, 0.1f, 0), 0.3f);

	debug_drawer->SetColor(Color(255, 0, 0, 200));
	debug_drawer->DrawSphere(path.back() + Vec3(0, 0.1f, 0), 0.3f);

	debug_drawer->BeginBatch();
	//DrawPathPoly(debug_drawer, *navmesh);
	if(test_path.smooth)
		DrawSmoothPath(debug_drawer, *navmesh, path);
	else
		DrawStraightPath(debug_drawer, *navmesh, path);
	debug_drawer->EndBatch();
}

void Navmesh::DrawStraightPath(DebugDrawer* debug_drawer, const dtNavMesh& mesh, const vector<Vec3>& path)
{
	debug_drawer->SetColor(Color(64, 16, 0, 220));
	for(int i = 0, length = (int)path.size() - 1; i < length; ++i)
		debug_drawer->AddLine(path[i], path[i + 1], 0.1f);
}

void Navmesh::DrawSmoothPath(DebugDrawer* debug_drawer, const dtNavMesh& mesh, const vector<Vec3>& path)
{
	debug_drawer->SetColor(Color(0, 0, 0, 220));
	Vec3 pts[2];
	int index = 0;
	for(const Vec3& pt : path)
	{
		pts[index] = pt;
		++index;
		if(index == 2)
		{
			index = 0;
			debug_drawer->AddLine(pts[0], pts[1], 0.1f);
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
		for(int j = 0; j < 3; ++j)
		{
			if(t[j] < poly->vertCount)
				debug_drawer->AddVertex(Vec3(&tile->verts[poly->verts[t[j]] * 3]));
			else
				debug_drawer->AddVertex(Vec3(&tile->detailVerts[(pd->vertBase + t[j] - poly->vertCount) * 3]));
		}
	}
}

void Navmesh::Save(FileWriter& f)
{
	const dtNavMesh& mesh = *navmesh;

	const int count = mesh.getMaxTiles();
	int tiles = 0;
	for(int i = 0; i < count; ++i)
	{
		const dtMeshTile* tile = mesh.getTile(i);
		if(tile && tile->header && tile->dataSize)
			++tiles;
	}
	f << tiles;

	for(int i = 0; i < count; ++i)
	{
		const dtMeshTile* tile = mesh.getTile(i);
		if(tile && tile->header && tile->dataSize)
		{
			f << mesh.getTileRef(tile);
			f << tile->dataSize;
			f.Write(tile->data, tile->dataSize);
		}
	}
}

void Navmesh::Load(FileReader& f)
{
	// call PrepareTiles first
	assert(navmesh && is_tiled);

	int tiles;
	f >> tiles;

	for(int i = 0; i < tiles; ++i)
	{
		dtTileRef tile_ref;
		int data_size;
		f >> tile_ref;
		f >> data_size;
		byte* data = (byte*)dtAlloc(data_size, DT_ALLOC_PERM);
		f.Read(data, data_size);

		dtStatus status = navmesh->addTile(data, data_size, DT_TILE_FREE_DATA, tile_ref, nullptr);
		if(dtStatusFailed(status))
		{
			dtFree(data);
			throw Format("Failed to load navmesh for tile %d.", i);
		}
	}
}
