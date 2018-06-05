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

class CityGenerator
{
public:
	void Init(Scene* scene, Level* level, Pathfinding* pathfinding, ResourceManager* res_mgr, uint size, uint splits);
	void Reset();
	void Generate();
	void DrawMap();
	float GetY(const Vec3& pos);
	float GetMapSize() { return map_size; }

	static const float tile_size;
	static const float floor_y;
	static const float wall_width;

private:
	void GenerateMap();
	void CreateScene();
	void SpawnItems();
	void SpawnZombies();
	Int2 PosToPt(const Vec3& pos);

	Scene* scene;
	Pathfinding* pathfinding;
	Level* level;
	vector<Tile> map;
	uint size;
	Mesh* mesh[T_MAX], *mesh_curb, *mesh_wall, *mesh_corner;
	float mesh_offset[T_MAX], map_size;
	vector<Building> buildings;
	Vec3 player_start_pos;
};
