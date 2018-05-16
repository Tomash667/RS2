#include "GameCore.h"
#include "CityGenerator.h"
#include <Scene.h>
#include <SceneNode.h>
#include <ResourceManager.h>
#include <ScenePart.h>
#include "Level.h"
#include "Player.h"

const float CityGenerator::tile_size = 5.f;
const float CityGenerator::floor_y = 0.05f;
const float CityGenerator::wall_width = 0.2f;

CityGenerator::CityGenerator() : map(nullptr)
{
}

CityGenerator::~CityGenerator()
{
	delete[] map;
}

void CityGenerator::Init(Scene* scene, Level* level, ResourceManager* res_mgr, uint size, uint splits)
{
	this->scene = scene;
	this->level = level;
	this->size = size;

	scene->InitQuadTree(tile_size * size, splits);
	map = new Tile[size * size];
	map_size = tile_size * size;

	mesh[T_ASPHALT] = res_mgr->GetMesh("asphalt.qmsh");
	mesh[T_PAVEMENT] = res_mgr->GetMesh("brick_pavement.qmsh");
	mesh[T_BUILDING] = res_mgr->GetMesh("building_floor_ceil.qmsh");

	mesh_offset[T_ASPHALT] = 0;
	mesh_offset[T_PAVEMENT] = floor_y;
	mesh_offset[T_BUILDING] = floor_y;

	mesh_curb = res_mgr->GetMesh("curb.qmsh");
	mesh_wall = res_mgr->GetMesh("wall.qmsh");
	mesh_corner = res_mgr->GetMesh("corner.qmsh");
}

void CityGenerator::Generate()
{
	const Vec3 player_pos(map_size / 2, 0, map_size / 2);	GenerateMap();
	CreateScene();
	level->SpawnBarriers();
	level->SpawnPlayer(player_start_pos);
	SpawnItems();
	SpawnZombies();
}

struct Leaf
{
	Leaf(const Int2& pos, const Int2& size) : pos(pos), size(size), child1(nullptr), child2(nullptr) {}
	bool CanSplit()
	{
		return size.x > max_size
			|| size.y > max_size
			|| (size.x >= min_size_before_split && size.y >= min_size_before_split && Rand() % 4 != 0);
	}
	void Split()
	{
		bool horizontal;
		float ratio = float(size.x) / size.y;
		if(ratio >= 1.25f)
			horizontal = true;
		else if(ratio <= 0.75f)
			horizontal = false;
		else
			horizontal = Rand() % 2 == 0;

		if(horizontal)
		{
			int split = Random(min_size, size.x - min_size - 1);
			line_pos = Int2(pos.x + split, pos.y);
			line_size = Int2(1, size.y);
			child1 = new Leaf(pos, Int2(split, size.y));
			child2 = new Leaf(Int2(pos.x + split + 1, pos.y), Int2(size.x - split - 1, size.y));
		}
		else
		{
			int split = Random(min_size, size.y - min_size - 1);
			line_pos = Int2(pos.x, pos.y + split);
			line_size = Int2(size.x, 1);
			child1 = new Leaf(pos, Int2(size.x, split));
			child2 = new Leaf(Int2(pos.x, pos.y + split + 1), Int2(size.x, size.y - split - 1));
		}
	}

	Int2 pos, size, line_pos, line_size;
	Leaf* child1, *child2;

	static const int max_size = 12;
	static const int min_size = 4;
	static const int min_size_before_split = min_size * 2 + 1;
};

void CityGenerator::GenerateMap()
{
	// init
	for(uint i = 0; i < size*size; ++i)
		map[i] = T_PAVEMENT;

	// generate roads
	vector<Leaf*> leafs, to_check;
	to_check.push_back(new Leaf(Int2(0, 0), Int2(size, size)));
	while(!to_check.empty())
	{
		Leaf* leaf = to_check.back();
		to_check.pop_back();
		leafs.push_back(leaf);
		if(leaf->CanSplit())
		{
			leaf->Split();
			to_check.push_back(leaf->child1);
			to_check.push_back(leaf->child2);

			for(int y = 0; y < leaf->line_size.y; ++y)
			{
				for(int x = 0; x < leaf->line_size.x; ++x)
					map[leaf->line_pos.x + x + (leaf->line_pos.y + y) * size] = T_ASPHALT;
			}
		}
	}

	// buildings
	const int building_min_size = 2;
	for(Leaf* leaf : leafs)
	{
		if(leaf->child1)
			continue;

		Int2 b_size(leaf->size.x - 2, leaf->size.y - 2);
		if(b_size.x != building_min_size && Rand() % 2 == 0)
			--b_size.x;
		if(b_size.y != building_min_size && Rand() % 2 == 0)
			--b_size.y;
		Int2 b_pos = leaf->pos + Int2(1 + Random(0, leaf->size.x - b_size.x - 2), 1 + Random(0, leaf->size.y - b_size.y - 2));

		for(int y = 0; y < b_size.y; ++y)
		{
			for(int x = 0; x < b_size.x; ++x)
				map[b_pos.x + x + (b_pos.y + y) * size] = T_BUILDING;
		}

		Building b;
		b.box = Box2d::Create(b_pos, b_size) * tile_size;
		b.pos = b_pos;
		b.size = b_size;
		buildings.push_back(b);
	}

	// player start pos
	Leaf* first = leafs.front();
	player_start_pos = Vec3(tile_size * first->line_pos.x + tile_size / 2 * first->line_size.x,
		0, tile_size * first->line_pos.y + tile_size / 2 * first->line_size.y);

	DeleteElements(leafs);
}

void CityGenerator::DrawMap()
{
	for(uint y = 0; y < size; ++y)
	{
		for(uint x = 0; x < size; ++x)
		{
			Tile tile = map[x + y * size];
			switch(tile)
			{
			case T_ASPHALT:
				putchar('#');
				break;
			case T_PAVEMENT:
				putchar('.');
				break;
			case T_BUILDING:
				putchar('|');
				break;
			}
		}
		putchar('\n');
	}
	putchar('\n');
}

void CityGenerator::CreateScene()
{
	Int2 pt, prev_pt = Int2(-1, -1);
	ScenePart* part = nullptr;
	QuadTree* quad_tree = scene->GetQuadTree();
	SceneNode* node;
	const float tile_size2 = tile_size / 2;

	// floor
	for(uint y = 0; y < size; ++y)
	{
		for(uint x = 0; x < size; ++x)
		{
			pt = quad_tree->PosToIndex(Vec2(tile_size * x + 1, tile_size * y + 1));
			assert(pt != Int2(-1, -1));
			if(pt != prev_pt)
			{
				prev_pt = pt;
				part = (ScenePart*)quad_tree->GetPart(pt);
			}

			// add floor mesh
			Tile tile = map[x + y * size];
			node = new SceneNode;
			node->pos = Vec3(tile_size * x, mesh_offset[tile], tile_size * y);
			node->rot = Vec3::Zero;
			node->mesh = mesh[tile];
			part->Add(node);

			if(tile == T_ASPHALT)
			{
				// add curb
				if(x > 0 && map[x - 1 + y * size] != T_ASPHALT)
				{
					// left
					node = new SceneNode;
					node->pos = Vec3(tile_size * x, 0, tile_size * y + tile_size2);
					node->rot = Vec3::Zero;
					node->mesh = mesh_curb;
					part->Add(node);
				}
				if(x < size - 1 && map[x + 1 + y * size] != T_ASPHALT)
				{
					// right
					node = new SceneNode;
					node->pos = Vec3(tile_size * (x + 1), 0, tile_size * y + tile_size2);
					node->rot = Vec3::Zero;
					node->mesh = mesh_curb;
					part->Add(node);
				}
				if(y > 0 && map[x + (y - 1) * size] != T_ASPHALT)
				{
					// bottom
					node = new SceneNode;
					node->pos = Vec3(tile_size * x + tile_size2, 0, tile_size * y);
					node->rot = Vec3(0, PI / 2, 0);
					node->mesh = mesh_curb;
					part->Add(node);
				}
				if(y < size - 1 && map[x + (y + 1) * size] != T_ASPHALT)
				{
					// top
					node = new SceneNode;
					node->pos = Vec3(tile_size * x + tile_size2, 0, tile_size * (y + 1));
					node->rot = Vec3(0, PI / 2, 0);
					node->mesh = mesh_curb;
					part->Add(node);
				}
			}
		}
	}

	// buildings
	for(Building& b : buildings)
	{
		SceneNode* building_node = new SceneNode;
		building_node->container = new SceneNode::Container;
		building_node->container->box = Box::CreateXZ(b.box, 0.f, 4.f);
		building_node->container->is_sphere = false;

		int doors;
		switch(Rand() % 10)
		{
		case 0:
		case 1:
			doors = 1;
			break;
		case 2:
		case 3:
		case 4:
		case 5:
			doors = 2;
			break;
		case 6:
		case 7:
		case 8:
			doors = 3;
			break;
		case 9:
			doors = 4;
			break;
		}

		int door_flags;
		switch(doors)
		{
		case 1:
			door_flags = 1 << (Rand() % 4);
			break;
		case 2:
			{
				int index = Rand() % 4;
				door_flags = (1 << index) | (1 << ((index + 1 + Rand() % 3) % 4));
			}
			break;
		case 3:
			door_flags = ~(1 << (Rand() % 4));
			break;
		case 4:
			door_flags = 0b1111;
			break;
		}

		Int2 size = b.size * 2;
		Int2 pos = b.pos * 2;
		int left_hole = IS_SET(door_flags, 1 << 0) ? (pos.y + Random(1, size.y - 2)) : -1;
		int right_hole = IS_SET(door_flags, 1 << 1) ? (pos.y + Random(1, size.y - 2)) : -1;
		int bottom_hole = IS_SET(door_flags, 1 << 2) ? (pos.x + Random(1, size.x - 2)) : -1;
		int top_hole = IS_SET(door_flags, 1 << 3) ? (pos.x + Random(1, size.x - 2)) : -1;

		for(int x = pos.x + 1; x < pos.x + size.x - 1; ++x)
		{
			// bottom wall
			if(x != bottom_hole)
			{
				node = new SceneNode;
				node->mesh = mesh_wall;
				node->pos = Vec3(tile_size2 * (x + 1), 0, tile_size2 * pos.y);
				node->rot = Vec3(0, PI / 2, 0);
				building_node->Add(node);
			}

			// top wall
			if(x != top_hole)
			{
				node = new SceneNode;
				node->mesh = mesh_wall;
				node->pos = Vec3(tile_size2 * (x + 1), 0, tile_size2 * (pos.y + size.y) - wall_width);
				node->rot = Vec3(0, PI / 2, 0);
				building_node->Add(node);
			}
		}

		for(int y = pos.y + 1; y < pos.y + size.y - 1; ++y)
		{
			// left wall
			if(y != left_hole)
			{
				node = new SceneNode;
				node->mesh = mesh_wall;
				node->pos = Vec3(tile_size2 * pos.x, 0, tile_size2 * y);
				node->rot = Vec3::Zero;
				building_node->Add(node);
			}

			// right wall
			if(y != right_hole)
			{
				node = new SceneNode;
				node->mesh = mesh_wall;
				node->pos = Vec3(tile_size2 * (pos.x + size.x) - wall_width, 0, tile_size2 * y);
				node->rot = Vec3::Zero;
				building_node->Add(node);
			}
		}

		// left bottom corner
		node = new SceneNode;
		node->mesh = mesh_corner;
		node->pos = Vec3(tile_size2 * pos.x, 0, tile_size2 * pos.y);
		node->rot = Vec3::Zero;
		building_node->Add(node);

		// left top corner
		node = new SceneNode;
		node->mesh = mesh_corner;
		node->pos = Vec3(tile_size2 * pos.x, 0, tile_size2 * (pos.y + size.y));
		node->rot = Vec3(0, -PI / 2, 0);
		building_node->Add(node);

		// right bottom corner
		node = new SceneNode;
		node->mesh = mesh_corner;
		node->pos = Vec3(tile_size2 * (pos.x + size.x), 0, tile_size2 * pos.y);
		node->rot = Vec3(0, PI / 2, 0);
		building_node->Add(node);

		// right top corner
		node = new SceneNode;
		node->mesh = mesh_corner;
		node->pos = Vec3(tile_size2 * (pos.x + size.x), 0, tile_size2 * (pos.y + size.y));
		node->rot = Vec3(0, PI, 0);
		building_node->Add(node);

		scene->Add(building_node);

		// colliders
		Collider c;
		c.half_size = Vec2(tile_size2 / 2, wall_width / 2);
		for(int x = pos.x; x < pos.x + size.x; ++x)
		{
			// bottom wall
			c.center = Vec2(tile_size2 * x + tile_size2 / 2, tile_size2 * pos.y + wall_width / 2);
			if(x != bottom_hole)
				level->AddCollider(c);
			
			// top wall
			if(x != top_hole)
			{
				c.center.y = tile_size2 * (pos.y + size.y) - wall_width / 2;
				level->AddCollider(c);
			}
		}
		c.half_size = Vec2(wall_width / 2, tile_size2 / 2);
		for(int y = pos.y; y < pos.y + size.y; ++y)
		{
			// left wall
			c.center = Vec2(tile_size2 * pos.x + wall_width / 2, tile_size2 * y + tile_size2 / 2);
			if(y != left_hole)
				level->AddCollider(c);

			// right wall
			if(y != right_hole)
			{
				c.center.x = tile_size2 * (pos.x + size.x) - wall_width / 2;
				level->AddCollider(c);
			}
		}

		level->camera_colliders.push_back(Box(tile_size2 * pos.x, 4.f, tile_size2 * pos.y,
			tile_size2 * (pos.x + size.x), 5.f, tile_size2 * (pos.y + size.y)));
	}
}

void CityGenerator::SpawnItems()
{
	// 50% chance to spawn in building
	for(Building& building : buildings)
	{
		if(Rand() % 2 == 0)
			continue;

		Vec2 pos = building.box.GetRandomPoint(2.f);
		level->SpawnMedkit(Vec3(pos.x, floor_y, pos.y));
	}
}

void CityGenerator::SpawnZombies()
{
	Vec2 player_pos = player_start_pos.XZ();
	for(int i = 0; i < 25; ++i)
	{
		for(int tries = 0; tries < 5; ++tries)
		{
			Vec2 pos = Vec2::Random(0, map_size);
			if(Vec2::Distance(player_pos, pos) < 10.f)
				continue;
			if(level->CheckCollision(nullptr, pos))
			{
				Vec3 spawn_pos(pos.x, 0, pos.y);
				spawn_pos.y = GetY(spawn_pos);
				level->SpawnZombie(spawn_pos);
				break;
			}
		}
	}
}

float CityGenerator::GetY(const Vec3& pos)
{
	Int2 pt = PosToPt(pos);
	if(pt == Int2(-1, -1))
		return 0;
	Tile tile = map[pt.x + pt.y * size];
	return mesh_offset[tile];
}

Int2 CityGenerator::PosToPt(const Vec3& pos)
{
	if(pos.x < 0 || pos.z < 0 || pos.x > map_size || pos.z > map_size)
		return Int2(-1, -1);
	return Int2(int(pos.x / tile_size), int(pos.z / tile_size));
}
