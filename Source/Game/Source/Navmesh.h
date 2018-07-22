#pragma once

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
	dtPolyRef GetPolyRef(const Vec3& pos);
	bool FindPath(const Vec3& from, const Vec3& to, vector<Vec3>& out_path);
	bool FindTestPath(const Vec3& from, const Vec3& to, bool smooth);
	void Draw(DebugDrawer* debug_drawer);
	void DrawPath(DebugDrawer* debug_drawer, const vector<Vec3>& path, bool smooth);
	void Save(FileWriter& f);
	void Load(FileReader& f);

private:
	struct TestPath
	{
		vector<Vec3> path;
		Vec3 start_pos, end_pos;
		dtPolyRef start_ref, end_ref;
		bool ok, smooth;
	};

	void Cleanup();
	void Reset();
	bool BuildTileMesh(const Int2& tile, const NavmeshGeometry& geom, byte*& data, int& data_size);
	void FindStraightPath(dtPolyRef end_ref, const Vec3& start_pos, const Vec3& end_pos, vector<Vec3>& out_path);
	void SmoothPath(dtPolyRef start_ref, const Vec3& start_pos, const Vec3& end_pos, vector<Vec3>& out_path);
	void DrawNavmesh(DebugDrawer* debug_drawer, const dtNavMesh& mesh);
	void DrawMeshTile(DebugDrawer* debug_drawer, const dtNavMesh& mesh, const dtMeshTile* tile);
	void DrawPolyBoundaries(DebugDrawer* debug_drawer, const dtMeshTile* tile, Color color, float line_width, bool inner);
	//void DrawPathPoly(DebugDrawer* debug_drawer, const dtNavMesh& mesh);
	void DrawStraightPath(DebugDrawer* debug_drawer, const dtNavMesh& mesh, const vector<Vec3>& path);
	void DrawSmoothPath(DebugDrawer* debug_drawer, const dtNavMesh& mesh, const vector<Vec3>& path);
	void DrawPoly(DebugDrawer* debug_drawer, const dtNavMesh& mesh, dtPolyRef ref, Color col);

	rcHeightfield* solid;
	rcCompactHeightfield* chf;
	rcContourSet* cset;
	rcPolyMesh* pmesh;
	rcPolyMeshDetail* dmesh;
	vector<byte> triareas;
	dtNavMeshQuery* nav_query;
	dtNavMesh* navmesh;
	dtQueryFilter* filter;
	float tile_size;
	uint tiles;
	bool is_tiled;

	// intermediate results
	static const int MAX_POLYS = 256;
	static const int MAX_SMOOTH = 2048;
	dtPolyRef tmp_path[MAX_POLYS];
	Vec3 tmp_straight_path[MAX_POLYS];
	int tmp_path_length;

	// test data
	TestPath test_path;
};
