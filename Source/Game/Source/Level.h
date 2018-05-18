#pragma once

#include "Collider.h"

class Level
{
public:
	Level();
	~Level();
	void Init(Scene* scene, ResourceManager* res_mgr, float level_size);
	void SpawnMedkit(const Vec3& pos);
	void SpawnZombie(const Vec3& pos);
	void SpawnPlayer(const Vec3& pos);
	void RemoveItem(GroundItem* item);
	bool CheckCollision(Unit* unit, const Vec2& pos);
	bool CheckCollision(Unit& unit, const Vec3& pos)
	{
		return CheckCollision(&unit, pos.XZ());
	}
	void AddCollider(const Collider& c);
	void SpawnBarriers();
	float RayTest(const Vec3& pos, const Vec3& ray);
	void SpawnBlood(const Vec3& pos, Mesh* mesh);
	void Update(float dt);

	Scene* scene;
	Player* player;
	vector<Zombie*> zombies;
	vector<GroundItem> items;
	vector<Box> camera_colliders;

private:
	Int2 PosToPt(const Vec2& pos)
	{
		return Int2(int(pos.x / tile_size), int(pos.y / tile_size));
	}

	ResourceManager* res_mgr;
	Mesh* mesh_medkit, *mesh_zombie;
	vector<vector<Collider>> colliders;
	vector<Collider> barriers;
	vector<SceneNode*> bloods;
	float level_size, tile_size;
	static const uint grids = 8;
};
