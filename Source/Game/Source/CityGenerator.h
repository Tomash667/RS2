#pragma once

#include "Collider.h"
#include "Building.h"

enum Tile
{
	T_ASPHALT,
	T_PAVEMENT,
	T_BUILDING,
	T_MAX
};

struct LevelGeometry
{
	vector<Vec3> verts;
	vector<int> tris;

	void SaveObj(cstring filename);
};

enum ThreadState
{
	THREAD_NOT_STARTED,
	THREAD_WORKING,
	THREAD_FINISHED,
	THREAD_QUIT
};

class CityGenerator
{
public:
	CityGenerator();
	~CityGenerator();
	void Init(Scene* scene, Level* level, ResourceManager* res_mgr, uint size, uint splits, Navmesh* navmesh);
	void Reset();
	void Generate();
	void DrawMap();
	float GetY(const Vec3& pos);
	float GetMapSize() { return map_size; }
	void Save(FileWriter& f);
	void Load(FileReader& f);
	void CheckNavmeshGeneration();
	void WaitForNavmeshThread();

	static const float tile_size;
	static const float floor_y;
	static const float wall_width;

private:
	void GenerateMap();
	void FillBuildings();
	void BuildBuildingsMesh();
	void CreateScene();
	void NavmeshThreadLoop();
	void BuildNavmesh();
	void BuildNavmeshTile(const Int2& tile, bool is_tiled);
	void SpawnItems();
	void SpawnItem(Building* building, Item* item);
	void SpawnZombies();
	Int2 PosToPt(const Vec3& pos);

	ResourceManager* res_mgr;
	Scene* scene;
	Navmesh* navmesh;
	Level* level;
	vector<Tile> map;
	uint size;
	float mesh_offset[T_MAX], map_size;
	vector<Building*> buildings;
	Vec3 player_start_pos;
	LevelGeometry geom;
	std::thread navmesh_thread;
	ThreadState navmesh_thread_state;
	int navmesh_built;
	Int2 navmesh_next_tile;
	Timer navmesh_timer;

	// resources
	Mesh* mesh[T_MAX], *mesh_curb, *mesh_table,
		*mesh_wall, *mesh_wall_inner, *mesh_corner, *mesh_door_jamb, *mesh_door_jamb_inner, *mesh_ceil, *mesh_roof;
};
