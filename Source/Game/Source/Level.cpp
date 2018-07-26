#include "GameCore.h"
#include "Level.h"
#include <ResourceManager.h>
#include <Scene.h>
#include <SceneNode.h>
#include <MeshInstance.h>
#include "GroundItem.h"
#include "Zombie.h"
#include "Player.h"
#include "Npc.h"
#include "Item.h"
#include "GameState.h"
#include <DebugDrawer.h>

Level::Level() : player(nullptr)
{
}

Level::~Level()
{
	DeleteElements(units);
}

void Level::Init(Scene* scene, ResourceManager* res_mgr, GameState* game_state, float level_size)
{
	this->scene = scene;
	this->res_mgr = res_mgr;
	this->game_state = game_state;
	this->level_size = level_size;
	tile_size = level_size / grids;

	colliders.resize(grids * grids);
	alive_zombies = 0;
	alive_npcs = 0;
}

void Level::Reset()
{
	DeleteElements(units);
	player = nullptr;
	items.clear();
	camera_colliders.clear();
	for(vector<Collider>& cols : colliders)
		cols.clear();
	barriers.clear();
	active_bloods.clear();
	bloods.clear();
	alive_zombies = 0;
	alive_npcs = 0;
}

void Level::LoadResources()
{
	mesh_human = res_mgr->GetMesh("units/human.qmsh");
	mesh_zombie = res_mgr->GetMesh("units/zombie.qmsh");
	mesh_hair = res_mgr->GetMesh("units/hair.qmsh");
	mesh_clothes = res_mgr->GetMesh("items/clothes.qmsh");
	mesh_blood_pool = res_mgr->GetMesh("particles/blood_pool.qmsh");
	mesh_zombie_blood_pool = res_mgr->GetMesh("particles/zombie_blood_pool.qmsh");
}

void Level::SpawnItem(const Vec3& pos, Item* item)
{
	assert(item && item->mesh);

	GroundItem ground_item;
	ground_item.item = item;
	ground_item.pos = pos;
	ground_item.rot = Random(PI * 2);
	ground_item.node = new SceneNode;
	ground_item.node->mesh = item->mesh;
	ground_item.node->use_matrix = true;
	ground_item.node->mat = Matrix::Rotation(-item->ground_rot.y, item->ground_rot.x, item->ground_rot.z)
		* Matrix::Translation(item->ground_offset)
		* Matrix::RotationY(-ground_item.rot)
		* Matrix::Translation(ground_item.pos);
	items.push_back(ground_item);
	scene->Add(ground_item.node);
}

void Level::SpawnZombie(const Vec3& pos)
{
	Zombie* zombie = CreateZombie();
	zombie->node->pos = pos;
	zombie->node->rot = Vec3(0, Random(PI * 2), 0);
	zombie->node->mesh_inst->Play("stoi", 0, 0);
	zombie->node->mesh_inst->SetToEnd();
	units.push_back(zombie);
	++alive_zombies;
}

Zombie* Level::CreateZombie()
{
	Zombie* zombie = new Zombie;
	zombie->node = new SceneNode;
	zombie->node->mesh = mesh_zombie;
	zombie->node->mesh_inst = new MeshInstance(mesh_zombie);
	scene->Add(zombie->node);
	return zombie;
}

void Level::SpawnPlayer(const Vec3& pos)
{
	player = CreatePlayer();
	player->node->mesh_inst->Play("stoi", 0, 0);
	player->node->mesh_inst->SetToEnd();
	player->node->pos = pos;
	player->node->rot = Vec3(0, PI / 2, 0); // north
	if(player->melee_weapon)
		player->weapon->mesh = player->melee_weapon->mesh;
	else
		player->weapon->visible = false;
	game_state->player = player;
	units.push_back(player);
}

Player* Level::CreatePlayer()
{
	Player* player = new Player(this);
	player->node = new SceneNode;
	player->node->mesh = mesh_human;
	player->node->mesh_inst = new MeshInstance(player->node->mesh);
	CLEAR_BIT(player->node->subs, 1 << 0); // don't draw body under clothes
	scene->Add(player->node);

	SceneNode* weapon = new SceneNode;
	weapon->pos = Vec3::Zero;
	weapon->rot = Vec3::Zero;
	player->node->Add(weapon, player->node->mesh->GetPoint("bron"));
	player->weapon = weapon;

	SceneNode* clothes = new SceneNode;
	clothes->mesh = mesh_clothes;
	clothes->pos = Vec3::Zero;
	clothes->rot = Vec3::Zero;
	player->node->Add(clothes, SceneNode::USE_PARENT_BONES);

	SceneNode* hair = new SceneNode;
	hair->mesh = mesh_hair;
	hair->pos = Vec3::Zero;
	hair->rot = Vec3::Zero;
	hair->tint = Color(86, 34, 0, 255);
	player->node->Add(hair, SceneNode::USE_PARENT_BONES);
	player->hair = hair;

	return player;
}

void Level::SpawnNpc(const Vec3& pos)
{
	Npc* npc = CreateNpc();
	npc->node->mesh_inst->Play("stoi", 0, 0);
	npc->node->mesh_inst->SetToEnd();
	npc->node->pos = pos;
	npc->node->rot = Vec3(0, Random(PI * 2), 0);
	units.push_back(npc);
	++alive_npcs;
}

Npc* Level::CreateNpc()
{
	Npc* npc = new Npc;
	npc->node = new SceneNode;
	npc->node->mesh = mesh_human;
	npc->node->mesh_inst = new MeshInstance(npc->node->mesh);
	CLEAR_BIT(npc->node->subs, 1 << 0); // don't draw body under clothes
	scene->Add(npc->node);

	SceneNode* weapon = new SceneNode;
	weapon->mesh = Item::Get("baseball_bat")->mesh;
	weapon->pos = Vec3::Zero;
	weapon->rot = Vec3::Zero;
	npc->node->Add(weapon, npc->node->mesh->GetPoint("bron"));
	npc->weapon = weapon;

	SceneNode* clothes = new SceneNode;
	clothes->mesh = mesh_clothes;
	clothes->pos = Vec3::Zero;
	clothes->rot = Vec3::Zero;
	npc->node->Add(clothes, SceneNode::USE_PARENT_BONES);

	SceneNode* hair = new SceneNode;
	hair->mesh = mesh_hair;
	hair->pos = Vec3::Zero;
	hair->rot = Vec3::Zero;
	hair->tint = Color(86, 34, 0, 255);
	npc->node->Add(hair, SceneNode::USE_PARENT_BONES);

	return npc;
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
	// units
	for(Unit* u : units)
	{
		if(u == unit || u->hp <= 0)
			continue;
		if(Distance(pos.x, pos.y, u->node->pos.x, u->node->pos.z) <= Unit::radius * 2)
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
		bool ignore = IS_SET(flags, COLLIDE_IGNORE_NO_BLOCK_VIEW);
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
					if(ignore && !c.block_view)
						continue;
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
		for(Unit* unit : units)
		{
			if(excluded != unit && unit->hp > 0)
			{
				if(RayToCylinder(pos, to, unit->node->pos, unit->node->pos.ModY(Unit::height), Unit::radius, t) && t > 0.f && t < min_t)
				{
					min_t = t;
					hit = unit;
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
	node->mesh = unit.type == UNIT_ZOMBIE ? mesh_zombie_blood_pool : mesh_blood_pool;
	node->pos = center;
	node->rot = Vec3(0, Random(PI * 2), 0);
	node->scale = 0.f;
	node->alpha = true;
	scene->Add(node);
	active_bloods.push_back(node);
}

void Level::Update(float dt)
{
	LoopRemove(active_bloods, [this, dt](SceneNode* node)
	{
		node->scale += dt;
		if(node->scale >= 1.f)
		{
			node->scale = 1.f;
			bloods.push_back({ node, 0 });
			return true;
		}
		else
			return false;
	});
}

void Level::GatherColliders(vector<Collider>& results, const Box2d& box)
{
	Rect rect(PosToPt(box.v1), PosToPt(box.v2));
	if(rect.p1.x >= (int)grids || rect.p1.y >= (int)grids || rect.p2.x < 0 || rect.p2.y < 0)
		return;
	rect.p1.x = max(rect.p1.x, 0);
	rect.p1.y = max(rect.p1.y, 0);
	rect.p2.x = min(rect.p2.x, (int)grids - 1);
	rect.p2.y = min(rect.p2.y, (int)grids - 1);
	for(int y = rect.p1.y; y <= rect.p2.y; ++y)
	{
		for(int x = rect.p1.x; x <= rect.p2.x; ++x)
		{
			for(Collider& c : colliders[x + y * grids])
			{
				if(RectangleToRectangle(box, c.ToBox2d()))
					results.push_back(c);
			}
		}
	}
}

void Level::Save(FileWriter& f)
{
	// units
	f << units.size();
	for(Unit* unit : units)
	{
		f << unit->type;
		unit->Save(f);
	}
	f << alive_zombies;
	f << alive_npcs;

	// ground items
	f << items.size();
	for(GroundItem& item : items)
	{
		f << item.item->id;
		f << item.pos;
		f << item.rot;
	}

	// bloods
	f << active_bloods.size();
	for(SceneNode* node : active_bloods)
	{
		f << node->pos;
		f << node->rot.y;
		f << node->scale;
		f << (node->mesh == mesh_blood_pool);
	}
	f << bloods.size();
	for(Blood& blood : bloods)
	{
		f << blood.node->pos;
		f << blood.node->rot.y;
		f << blood.node->tint.w;
		f << (blood.node->mesh == mesh_blood_pool);
		f << blood.timer;
	}
}

void Level::Load(FileReader& f)
{
	// units
	uint count;
	f >> count;
	units.reserve(count);
	for(uint i = 0; i < count; ++i)
	{
		UnitType type;
		f >> type;
		switch(type)
		{
		case UNIT_PLAYER:
			{
				player = CreatePlayer();
				player->Load(f);
				game_state->player = player;
				units[i] = player;
			}
			break;
		case UNIT_ZOMBIE:
			{
				Zombie* zombie = CreateZombie();
				zombie->Load(f);
				units[i] = zombie;
			}
			break;
		case UNIT_NPC:
			{
				Npc* npc = CreateNpc();
				npc->Load(f);
				units[i] = npc;
			}
			break;
		}
	}
	f >> alive_zombies;
	f >> alive_npcs;

	// ground items
	items.resize(f.Read<uint>());
	for(GroundItem& ground_item : items)
	{
		ground_item.item = Item::Get(f.ReadString1());
		f >> ground_item.pos;
		f >> ground_item.rot;
		ground_item.node = new SceneNode;
		ground_item.node->mesh = ground_item.item->mesh;
		ground_item.node->use_matrix = true;
		ground_item.node->mat = Matrix::Rotation(-ground_item.item->ground_rot.y, ground_item.item->ground_rot.x, ground_item.item->ground_rot.z)
			* Matrix::Translation(ground_item.item->ground_offset)
			* Matrix::RotationY(-ground_item.rot)
			* Matrix::Translation(ground_item.pos);
		scene->Add(ground_item.node);
	}

	// bloods
	f >> count;
	active_bloods.reserve(count);
	for(uint i = 0; i < count; ++i)
	{
		SceneNode* node = new SceneNode;
		f >> node->pos;
		f >> node->rot.y;
		node->rot.x = 0;
		node->rot.z = 0;
		f >> node->scale;
		node->mesh = (f.Read<bool>() ? mesh_blood_pool : mesh_zombie_blood_pool);
		node->alpha = true;
		active_bloods.push_back(node);
		scene->Add(node);
	}
	f >> count;
	bloods.reserve(count);
	for(uint i = 0; i < count; ++i)
	{
		SceneNode* node = new SceneNode;
		f >> node->pos;
		f >> node->rot.y;
		node->rot.x = 0;
		node->rot.z = 0;
		f >> node->tint.w;
		node->mesh = (f.Read<bool>() ? mesh_blood_pool : mesh_zombie_blood_pool);
		node->alpha = true;
		int timer;
		f >> timer;
		bloods.push_back({ node, timer });
		scene->Add(node);
	}
}

void Level::DrawColliders(DebugDrawer* debug_drawer)
{
	debug_drawer->SetWireframe(true);
	debug_drawer->SetColor(Color(255, 0, 0));
	for(vector<Collider>& vc : colliders)
	{
		for(Collider& c : vc)
			debug_drawer->DrawCube(c.ToBox());
	}
}
