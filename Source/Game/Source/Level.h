#pragma once

#include "Collider.h"

class Level
{
public:
	Level();
	~Level();
	void Init(Scene* scene, ResourceManager* res_mgr, float level_size);
	void LoadResources();
	void SpawnItem(const Vec3& pos, Item* item);
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
	void SpawnBlood(Unit& unit);
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
	Mesh* mesh_zombie, *mesh_blood_pool, *mesh_zombie_blood_pool;
	vector<vector<Collider>> colliders;
	vector<Collider> barriers;
	vector<SceneNode*> bloods;
	float level_size, tile_size;
	static const uint grids = 8;
};
