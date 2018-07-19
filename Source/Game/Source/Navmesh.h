#pragma once

// FIXME - hide implementation details
#include <Recast.h>
#include <DetourNavMesh.h>
#include <DetourNavMeshQuery.h>

struct NavmeshGeometry
{
	Vec3* verts;
	int* tris;
	Box bounds;
	uint vert_count, tri_count;
};

class Navmesh
{
public:
	Navmesh();
	~Navmesh();
	bool PrepareTiles(float tile_size, uint tiles);
	bool Build(const NavmeshGeometry& geom);
	bool BuildTile(const Int2& tile, const NavmeshGeometry& geom);
	Box2d GetBoxForTile(const Int2& tile);
	void FindPath(const Vec3& from, const Vec3& to);
	void Draw(DebugDrawer* debug_drawer);

	// FIXME
	float dt;
	Input* input;
	void SetPos(const Vec3& pos, bool start);

private:
	void Cleanup();
	void Reset();
	bool BuildTileMesh(const Int2& tile, const NavmeshGeometry& geom, byte*& data, int& data_size);
	void FindStraightPath();
	void SmoothPath();
	void DrawNavmesh(DebugDrawer* debug_drawer, const dtNavMesh& mesh, const dtNavMeshQuery& query);
	void DrawMeshTile(DebugDrawer* debug_drawer, const dtNavMesh& mesh, const dtNavMeshQuery& query, const dtMeshTile* tile);
	void DrawPolyBoundaries(DebugDrawer* debug_drawer, const dtMeshTile* tile, Color color, float line_width, bool inner);
	void DrawPathPoly(DebugDrawer* debug_drawer, const dtNavMesh& mesh);
	void DrawStraightPath(DebugDrawer* debug_drawer, const dtNavMesh& mesh);
	void DrawSmoothPath(DebugDrawer* debug_drawer, const dtNavMesh& mesh);
	void DrawPoly(DebugDrawer* debug_drawer, const dtNavMesh& mesh, dtPolyRef ref, Color col);

	//
	rcHeightfield* solid;
	rcCompactHeightfield* chf;
	rcContourSet* cset;
	rcPolyMesh* pmesh;
	rcPolyMeshDetail* dmesh;
	vector<byte> triareas;
	//
	dtNavMeshQuery* nav_query;
	dtNavMesh* navmesh;
	dtQueryFilter filter;

	float tile_size;
	uint tiles;
	bool is_tiled;

	// FIXME
	bool start_set, end_set, have_path;
	Vec3 start_pos, end_pos;
	static const int MAX_POLYS = 256;
	static const int MAX_SMOOTH = 2048;
	dtPolyRef path[MAX_POLYS], start_ref, end_ref, straight_path_polys[MAX_POLYS];
	vector<Vec3> smooth_path;
	Vec3 straight_path[MAX_POLYS];
	int path_length, straight_path_length;
	byte straight_path_flags[MAX_POLYS];
};
