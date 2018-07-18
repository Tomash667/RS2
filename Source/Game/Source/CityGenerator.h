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

class CityGenerator
{
public:
	~CityGenerator();
	void Init(Scene* scene, Level* level, Pathfinding* pathfinding, ResourceManager* res_mgr, uint size, uint splits, Navmesh* navmesh);
	void Reset();
	void Generate();
	void DrawMap();
	float GetY(const Vec3& pos);
	float GetMapSize() { return map_size; }
	void Save(FileWriter& f);
	void Load(FileReader& f);

	static const float tile_size;
	static const float floor_y;
	static const float wall_width;

private:
	void GenerateMap();
	void FillBuildings();
	void BuildBuildingsMesh();
	void CreateScene();
	void BuildNavmesh();
	void BuildNavmeshTile(const Int2& tile);
	void SpawnItems();
	void SpawnItem(Building* building, Item* item);
	void SpawnZombies();
	Int2 PosToPt(const Vec3& pos);

	ResourceManager* res_mgr;
	Scene* scene;
	Pathfinding* pathfinding;
	Navmesh* navmesh;
	Level* level;
	vector<Tile> map;
	uint size;
	float mesh_offset[T_MAX], map_size;
	vector<Building*> buildings;
	Vec3 player_start_pos;
	LevelGeometry geom;

	// resources
	Mesh* mesh[T_MAX], *mesh_curb, *mesh_table,
		*mesh_wall, *mesh_wall_inner, *mesh_corner, *mesh_door_jamb, *mesh_door_jamb_inner, *mesh_ceil, *mesh_roof;
};
