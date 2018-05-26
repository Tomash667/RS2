#include "GameCore.h"
#include "Level.h"
#include <ResourceManager.h>
#include <Scene.h>
#include <SceneNode.h>
#include <MeshInstance.h>
#include "GroundItem.h"
#include "Zombie.h"
#include "Player.h"
#include "Item.h"

Level::Level() : player(nullptr)
{
}

Level::~Level()
{
	delete player;
	DeleteElements(zombies);
}

void Level::Init(Scene* scene, ResourceManager* res_mgr, float level_size)
{
	this->scene = scene;
	this->res_mgr = res_mgr;
	this->level_size = level_size;
	tile_size = level_size / grids;

	colliders.resize(grids * grids);
}

void Level::Reset()
{
	delete player;
	player = nullptr;
	DeleteElements(zombies);
	items.clear();
	camera_colliders.clear();
	for(vector<Collider>& cols : colliders)
		cols.clear();
	barriers.clear();
	bloods.clear();
}

void Level::LoadResources()
{
	mesh_zombie = res_mgr->GetMesh("zombie.qmsh");
	mesh_blood_pool = res_mgr->GetMesh("blood_pool.qmsh");
	mesh_zombie_blood_pool = res_mgr->GetMesh("zombie_blood_pool.qmsh");
}

void Level::SpawnItem(const Vec3& pos, Item* item)
{
	assert(item && item->mesh);
	SceneNode* node = new SceneNode;
	node->mesh = item->mesh;
	node->pos = pos + item->ground_offset;
	node->rot = item->ground_rot;
	scene->Add(node);

	GroundItem ground_item;
	ground_item.node = node;
	ground_item.item = item;
	ground_item.pos = pos;
	items.push_back(ground_item);
}

void Level::SpawnZombie(const Vec3& pos)
{
	Zombie* zombie = new Zombie;
	zombie->node = new SceneNode;
	zombie->node->pos = pos;
	zombie->node->rot = Vec3(0, Random(PI * 2), 0);
	zombie->node->mesh = mesh_zombie;
	zombie->node->mesh_inst = new MeshInstance(mesh_zombie);
	zombie->node->mesh_inst->Play("stoi", 0, 0);
	zombie->node->mesh_inst->SetToEnd();
	zombies.push_back(zombie);

	scene->Add(zombie->node);
}

void Level::SpawnPlayer(const Vec3& pos)
{
	player = new Player(this);
	player->node = new SceneNode;
	player->node->mesh = res_mgr->GetMesh("human.qmsh");
	player->node->mesh_inst = new MeshInstance(player->node->mesh);
	player->node->mesh_inst->Play("stoi", 0, 0);
	player->node->mesh_inst->SetToEnd();
	player->node->pos = pos;
	player->node->rot = Vec3(0, PI / 2, 0); // north
	CLEAR_BIT(player->node->subs, 1 << 0); // don't draw body under clothes
	scene->Add(player->node);

	SceneNode* weapon = new SceneNode;
	if(player->melee_weapon)
		weapon->mesh = player->melee_weapon->mesh;
	else
		weapon->visible = false;
	weapon->pos = Vec3::Zero;
	weapon->rot = Vec3::Zero;
	player->node->Add(weapon, player->node->mesh->GetPoint("bron"));
	player->weapon = weapon;

	SceneNode* clothes = new SceneNode;
	clothes->mesh = res_mgr->GetMesh("clothes.qmsh");
	clothes->pos = Vec3::Zero;
	clothes->rot = Vec3::Zero;
	player->node->Add(clothes, SceneNode::USE_PARENT_BONES);

	SceneNode* hair = new SceneNode;
	hair->mesh = res_mgr->GetMesh("hair.qmsh");
	hair->pos = Vec3::Zero;
	hair->rot = Vec3::Zero;
	hair->tint = Color(86, 34, 0);
	player->node->Add(hair, SceneNode::USE_PARENT_BONES);
	player->hair = hair;
}

void Level::RemoveItem(GroundItem* item)
{
	assert(item);
	for(auto it = items.begin(), end = items.end(); it != end; ++it)
	{
		if(&*it == item)
		{
			scene->Remove(it->node);
			items.erase(it);
			break;
		}
	}
}

bool Level::CheckCollision(Unit* unit, const Vec2& pos)
{
	// player
	if(unit != player && player->hp > 0)
	{
		if(Distance(pos.x, pos.y, player->node->pos.x, player->node->pos.z) <= Unit::radius * 2)
			return false;
	}

	// zombies
	for(Zombie* zombie : zombies)
	{
		if(zombie == unit || zombie->hp <= 0)
			continue;
		if(Distance(pos.x, pos.y, zombie->node->pos.x, zombie->node->pos.z) <= Unit::radius * 2)
			return false;
	}

	// barriers
	for(Collider& c : barriers)
	{
		if(CircleToRectangle(pos.x, pos.y, Unit::radius, c.center.x, c.center.y, c.half_size.x, c.half_size.y))
			return false;
	}

	// colliders
	Int2 pt = PosToPt(pos);
	int minx = max(0, pt.x - 1),
		miny = max(0, pt.y - 1),
		maxx = min((int)grids - 1, pt.x + 1),
		maxy = min((int)grids - 1, pt.y + 1);
	for(int y = miny; y <= maxy; ++y)
	{
		for(int x = minx; x <= maxx; ++x)
		{
			for(Collider& c : colliders[x + y * grids])
			{
				if(CircleToRectangle(pos.x, pos.y, Unit::radius, c.center.x, c.center.y, c.half_size.x, c.half_size.y))
					return false;
			}
		}
	}

	return true;
}

void Level::AddCollider(const Collider& c)
{
	Int2 pt = PosToPt(c.center);
	assert(pt.x >= 0 && pt.y >= 0 && pt.x < grids && pt.y < grids);
	colliders[pt.x + pt.y * grids].push_back(c);
}

void Level::SpawnBarriers()
{
	// left
	barriers.push_back(Collider(Vec2(-0.5f, level_size / 2), Vec2(0.5f, level_size / 2)));
	// right
	barriers.push_back(Collider(Vec2(level_size + 0.5f, level_size / 2), Vec2(0.5f, level_size / 2)));
	// bottom
	barriers.push_back(Collider(Vec2(level_size / 2, -0.5f), Vec2(level_size / 2, 0.5f)));
	// top
	barriers.push_back(Collider(Vec2(level_size / 2, level_size + 0.5f), Vec2(level_size / 2, 0.5f)));

	camera_colliders.push_back(Box(0, -1, 0, level_size, 0.05f, level_size));
}

bool Level::RayTest(const Vec3& pos, const Vec3& ray, float& out_t, int flags, Unit* excluded, Unit** target)
{
	float t, min_t = 2.f;
	Unit* hit = nullptr;

	if(IS_SET(flags, COLLIDE_COLLIDERS))
	{
		Int2 pt1 = PosToPt(pos.XZ()),
			pt2 = PosToPt((pos + ray).XZ());
		Int2::MinMax(pt1, pt2);
		int minx = max(0, pt1.x),
			miny = max(0, pt1.y),
			maxx = min((int)grids - 1, pt2.x),
			maxy = min((int)grids - 1, pt2.y);

		for(int y = miny; y <= maxy; ++y)
		{
			for(int x = minx; x <= maxx; ++x)
			{
				for(Collider& c : colliders[x + y * grids])
				{
					const Box box = c.ToBox();
					if(RayToBox(pos, ray, box, &t) && t > 0.f && t < min_t)
						min_t = t;
				}
			}
		}
	}

	if(IS_SET(flags, COLLIDE_BOXES))
	{
		for(Box& c : camera_colliders)
		{
			if(RayToBox(pos, ray, c, &t) && t > 0.f && t < min_t)
				min_t = t;
		}
	}

	if(IS_SET(flags, COLLIDE_UNITS))
	{
		Vec3 to = pos + ray;

		if(excluded != player && player->hp > 0)
		{
			if(RayToCylinder(pos, to, player->node->pos, player->node->pos.ModY(Unit::height), Unit::radius, t) && t > 0.f && t < min_t)
			{
				min_t = t;
				hit = player;
			}
		}

		for(Zombie* zombie : zombies)
		{
			if(excluded != zombie && zombie->hp > 0)
			{
				if(RayToCylinder(pos, to, zombie->node->pos, zombie->node->pos.ModY(Unit::height), Unit::radius, t) && t > 0.f && t < min_t)
				{
					min_t = t;
					hit = zombie;
				}
			}
		}
	}

	if(target)
		*target = hit;
	if(min_t > 1.f)
	{
		out_t = 1.f;
		return false;
	}
	else
	{
		out_t = min_t;
		return true;
	}
}

void Level::SpawnBlood(Unit& unit)
{
	unit.node->mesh_inst->SetupBones();

	Vec3 center;
	Mesh::Point* point = unit.node->mesh->GetPoint("centrum");
	if(!point)
		center = unit.node->pos.ModY(0.05f);
	else
	{
		Matrix mat = point->mat
			* unit.node->mesh_inst->GetMatrixBones()[point->bone]
			* (Matrix::RotationY(-unit.node->rot.y) * Matrix::Translation(unit.node->pos));
		center = Vec3::TransformZero(mat);
		center.y = unit.node->pos.y + 0.05f;
	}

	SceneNode* node = new SceneNode;
	node->mesh = unit.is_zombie ? mesh_zombie_blood_pool : mesh_blood_pool;
	node->pos = center;
	node->rot = Vec3(0, Random(PI * 2), 0);
	node->scale = 0.f;
	node->alpha = true;
	scene->Add(node);
	bloods.push_back(node);
}

void Level::Update(float dt)
{
	LoopRemove(bloods, [dt](SceneNode* node)
	{
		node->scale += dt;
		if(node->scale >= 1.f)
		{
			node->scale = 1.f;
			return true;
		}
		else
			return false;
	});
}
