#include "GameCore.h"
#include "CityGenerator.h"
#include <Scene.h>
#include <SceneNode.h>
#include <ResourceManager.h>
#include <ScenePart.h>
#include "Level.h"
#include "Player.h"
#include "Item.h"
#include "Pathfinding.h"
#include "Tree.h"
#include <Mesh.h>

const float CityGenerator::tile_size = 5.f;
const float CityGenerator::floor_y = 0.05f;
const float CityGenerator::wall_width = 0.2f;

CityGenerator::~CityGenerator()
{
	DeleteElements(buildings);
}

void CityGenerator::Init(Scene* scene, Level* level, Pathfinding* pathfinding, ResourceManager* res_mgr, uint size, uint splits)
{
	this->res_mgr = res_mgr;
	this->scene = scene;
	this->level = level;
	this->pathfinding = pathfinding;
	this->size = size;

	scene->InitQuadTree(tile_size * size, splits);
	map.resize(size * size);
	map_size = tile_size * size;

	mesh[T_ASPHALT] = res_mgr->GetMesh("asphalt.qmsh");
	mesh[T_PAVEMENT] = res_mgr->GetMesh("brick_pavement.qmsh");
	mesh[T_BUILDING] = res_mgr->GetMesh("building_floor.qmsh");

	mesh_offset[T_ASPHALT] = 0;
	mesh_offset[T_PAVEMENT] = floor_y;
	mesh_offset[T_BUILDING] = floor_y;

	mesh_curb = res_mgr->GetMesh("curb.qmsh");
	mesh_wall = res_mgr->GetMesh("wall.qmsh");
	mesh_corner = res_mgr->GetMesh("corner.qmsh");

	tex_ceil = res_mgr->GetTexture("ceil.jpg");
	tex_wall = res_mgr->GetTexture("wall.jpg");
	tex_wall_inner = res_mgr->GetTexture("wall_inner.jpg");
}

void CityGenerator::Reset()
{
	DeleteElements(buildings);
	scene->Reset();
	level->Reset();
}

void CityGenerator::Generate()
{
	GenerateMap();
	FillBuildings();
	CreateScene();
	level->SpawnBarriers();
	level->SpawnPlayer(player_start_pos);
	SpawnItems();
	//SpawnZombies(); FIXME
	pathfinding->GenerateBlockedGrid(size, tile_size, buildings);
}

void CityGenerator::GenerateMap()
{
	// init
	for(uint i = 0; i < size*size; ++i)
		map[i] = T_PAVEMENT;

	// generate roads
	Tree roads_tree(4, 12, Int2(size), 1);
	roads_tree.SplitAll();
	for(Tree::Node* node : roads_tree.nodes)
	{
		for(int y = 0; y < node->split_size.y; ++y)
		{
			for(int x = 0; x < node->split_size.x; ++x)
				map[node->split_pos.x + x + (node->split_pos.y + y) * size] = T_ASPHALT;
		}
	}

	// buildings
	const int building_min_size = 2;
	for(Tree::Node* node : roads_tree.leafs)
	{
		Int2 b_size(node->size.x - 2, node->size.y - 2);
		if(b_size.x != building_min_size && Rand() % 2 == 0)
			--b_size.x;
		if(b_size.y != building_min_size && Rand() % 2 == 0)
			--b_size.y;
		Int2 b_pos = node->pos + Int2(1 + Random(0, node->size.x - b_size.x - 2), 1 + Random(0, node->size.y - b_size.y - 2));

		for(int y = 0; y < b_size.y; ++y)
		{
			for(int x = 0; x < b_size.x; ++x)
				map[b_pos.x + x + (b_pos.y + y) * size] = T_BUILDING;
		}

		Building* b = new Building;
		b->box = Box2d::Create(b_pos, b_size) * tile_size;
		b->pos = b_pos;
		b->size = b_size;
		buildings.push_back(b);
	}

	// player start pos
	Tree::Node* first = roads_tree.nodes.front();
	player_start_pos = Vec3(tile_size * first->split_pos.x + tile_size / 2 * first->split_size.x,
		0, tile_size * first->split_pos.y + tile_size / 2 * first->split_size.y);
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

void CityGenerator::FillBuildings()
{
	vector<uint> indices;
	vector<Building::Room*> outside_rooms, rooms_to_check;
	MeshInfo info;
	info.subs.resize(3);
	info.subs[0].tex = tex_ceil;
	info.subs[1].tex = tex_wall;
	info.subs[2].tex = tex_wall_inner;
	//info.subs[3].tex = tex_roof;

	for(Building* building : buildings)
	{
		const Int2 size = building->size * 2;

		Tree tree(3, 6, size, 0);
		tree.SplitAll();

		indices.resize(size.x * size.y);
		outside_rooms.clear();

		building->rooms.resize(tree.leafs.size());
		for(uint i = 0, count = tree.leafs.size(); i < count; ++i)
		{
			// copy room data from node
			Building::Room& room = building->rooms[i];
			Tree::Node* node = tree.leafs[i];
			room.pos = node->pos;
			room.size = node->size;
			room.outside = 0;
			room.outside_used = 0;
			room.visited = false;

			// mark indices
			for(int y = room.pos.y; y < room.pos.y + room.size.y; ++y)
			{
				for(int x = room.pos.x; x < room.pos.x + room.size.x; ++x)
					indices[x + y * size.x] = i;
			}
		}

		// list connected rooms/outside
		for(Building::Room& room : building->rooms)
		{
			uint last = 9999;

			// left
			if(room.pos.x != 0)
			{
				for(uint y = room.pos.y, count = room.pos.y + room.size.y; y < count; ++y)
					building->CheckConnect(room, indices[room.pos.x - 1 + y * size.x], last);
			}
			else
				room.outside |= DIR_F_LEFT;

			// right
			if(room.pos.x + room.size.x != size.x)
			{
				for(uint y = room.pos.y, count = room.pos.y + room.size.y; y < count; ++y)
					building->CheckConnect(room, indices[room.pos.x + room.size.x + y * size.x], last);
			}
			else
				room.outside |= DIR_F_RIGHT;

			// bottom
			if(room.pos.y != 0)
			{
				for(uint x = room.pos.x, count = room.pos.x + room.size.x; x < count; ++x)
					building->CheckConnect(room, indices[x + (room.pos.y - 1) * size.x], last);
			}
			else
				room.outside |= DIR_F_BOTTOM;

			// top
			if(room.pos.y + room.size.y != size.y)
			{
				for(uint x = room.pos.x, count = room.pos.x + room.size.x; x < count; ++x)
					building->CheckConnect(room, indices[x + (room.pos.y + room.size.y) * size.x], last);
			}
			else
				room.outside |= DIR_F_TOP;

			if(room.outside != 0)
				outside_rooms.push_back(&room);
		}

		// random count of doors
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
		if(size.x * size.y > 30)
		{
			++doors;
			if(size.x * size.y > 60)
				++doors;
		}

		// add outside doors
		for(int i = 0; i < doors && !outside_rooms.empty(); ++i)
		{
			Building::Room* room = outside_rooms[Rand() % outside_rooms.size()];
			int j = Rand() % 4;
			while(true)
			{
				if(IS_SET(room->outside, 1 << j) && !IS_SET(room->outside_used, 1 << j))
				{
					Rect rect;
					switch(j)
					{
					case DIR_LEFT:
						rect = Rect(room->pos.x, room->pos.y, room->pos.x, room->pos.y + room->size.y - 1);
						break;
					case DIR_RIGHT:
						rect = Rect(room->pos.x + room->size.x - 1, room->pos.y, room->pos.x + room->size.x - 1, room->pos.y + room->size.y - 1);
						break;
					case DIR_BOTTOM:
						rect = Rect(room->pos.x, room->pos.y, room->pos.x + room->size.x - 1, room->pos.y);
						break;
					case DIR_TOP:
						rect = Rect(room->pos.x, room->pos.y + room->size.y - 1, room->pos.x + room->size.x - 1, room->pos.y + room->size.y - 1);
						break;
					}

					Int2 pt;
					if(rect.p1.x == rect.p2.x)
					{
						pt.x = rect.p1.x;
						pt.y = Random(rect.p1.y, rect.p2.y);
						if(pt.y == rect.p1.y || pt.y == rect.p2.y)
							pt.y = Random(rect.p1.y, rect.p2.y);
					}
					else
					{
						pt.y = rect.p1.y;
						pt.x = Random(rect.p1.x, rect.p2.x);
						if(pt.x == rect.p1.x || pt.x == rect.p2.x)
							pt.x = Random(rect.p1.x, rect.p2.x);
					}
					building->doors.push_back(std::make_pair(pt, (DIR)j));

					room->outside_used |= 1 << j;
					if(room->outside_used == room->outside)
						RemoveElement(outside_rooms, room);
					break;
				}
				j = (j + 1) % 4;
			}
		}

		// connect rooms
		for(uint i = 0, count = building->rooms.size(); i < count; ++i)
		{
			Building::Room& room = building->rooms[i];
			for(uint j : room.connected)
			{
				if(i < j && Rand() % 2 == 0)
				{
					room.connected2.push_back(j);
					building->rooms[j].connected2.push_back(i);
				}
			}
		}

		// verify all rooms are connected
		rooms_to_check.clear();
		rooms_to_check.push_back(&building->rooms.front());
		building->rooms.front().visited = true;
		while(!rooms_to_check.empty())
		{
			Building::Room* room = rooms_to_check.back();
			rooms_to_check.pop_back();
			for(uint i : room->connected2)
			{
				Building::Room* room2 = &building->rooms[i];
				if(room2->visited)
					continue;
				rooms_to_check.push_back(room2);
				room2->visited = true;
			}
		}
		bool all_ok = false;
		while(!all_ok)
		{
			all_ok = true;
			uint my_index = 0;
			for(Building::Room& room : building->rooms)
			{
				if(room.visited)
				{
					++my_index;
					continue;
				}

				uint index = Rand() % room.connected.size(), start_index = index;
				bool ok = false;
				do
				{
					uint i = room.connected[index];
					Building::Room& room2 = building->rooms[i];
					if(room2.visited)
					{
						room.visited = true;
						room.connected2.push_back(i);
						room2.connected2.push_back(my_index);
						ok = true;
						break;
					}
					index = (index + 1) % room.connected.size();
				}
				while(index != start_index);

				if(!ok)
					all_ok = false;
				++my_index;
			}
		}

		// set is_doors map
		building->is_doors.resize(size.x * size.y, 0);
		for(std::pair<Int2, DIR>& door : building->doors)
			building->is_doors[door.first.x + door.first.y * size.x] |= 1 << door.second;

		// build mesh
		vector<Vertex>& v = info.vertices;
		v.clear();
		info.indices.clear();
		Vec3 offset = Vec3(-tile_size * building->size.x / 2, 0, -tile_size * building->size.y / 2);
		// ceiling
		info.subs[0].first = 0;
		const float h = 4.f;
		const float ts = tile_size / 2;
		v.push_back(Vertex(Vec3(0, h, 0) + offset, Vec3(0, -1, 0), Vec2(0, 0)));
		v.push_back(Vertex(Vec3(ts*size.x, h, 0) + offset, Vec3(0, -1, 0), Vec2((float)size.x, 0)));
		v.push_back(Vertex(Vec3(0, h, ts*size.y) + offset, Vec3(0, -1, 0), Vec2(0, (float)size.y)));
		v.push_back(Vertex(Vec3(ts*size.x, h, ts*size.y) + offset, Vec3(0, -1, 0), Vec2((float)size.x, (float)size.y)));
		info.subs[0].tris = 2;
		// outside wall
		const Vec2 uv(1.f, 1.f);
		info.subs[1].first = v.size() / 2;
		for(int x = 0; x < size.x; ++x)
		{
			// bottom
			if(!building->IsDoors(Int2(x, 0), DIR_BOTTOM))
			{
				v.push_back(Vertex(Vec3(ts*x, h, 0) + offset, Vec3(0, 0, -1), Vec2(0, uv.y)));
				v.push_back(Vertex(Vec3(ts*(x + 1), h, 0) + offset, Vec3(0, 0, -1), Vec2(uv.x, uv.y)));
				v.push_back(Vertex(Vec3(ts*x, 0, 0) + offset, Vec3(0, 0, -1), Vec2(0, 0)));
				v.push_back(Vertex(Vec3(ts*(x + 1), 0, 0) + offset, Vec3(0, 0, -1), Vec2(uv.x, 0)));
			}
			// top
			if(!building->IsDoors(Int2(x, size.y - 1), DIR_TOP))
			{
				v.push_back(Vertex(Vec3(ts*(x + 1), h, ts*size.y) + offset, Vec3(0, 0, 1), Vec2(0, uv.y)));
				v.push_back(Vertex(Vec3(ts*x, h, ts*size.y) + offset, Vec3(0, 0, 1), Vec2(uv.x, uv.y)));
				v.push_back(Vertex(Vec3(ts*(x + 1), 0, ts*size.y) + offset, Vec3(0, 0, 1), Vec2(0, 0)));
				v.push_back(Vertex(Vec3(ts*x, 0, ts*size.y) + offset, Vec3(0, 0, 1), Vec2(uv.x, 0)));
			}
		}
		for(int y = 0; y < size.y; ++y)
		{
			// left
			if(!building->IsDoors(Int2(0, y), DIR_LEFT))
			{
				v.push_back(Vertex(Vec3(0, h, ts*(y+1)) + offset, Vec3(-1, 0, 0), Vec2(0, uv.y)));
				v.push_back(Vertex(Vec3(0, h, ts*y) + offset, Vec3(-1, 0, 0), Vec2(uv.x, uv.y)));
				v.push_back(Vertex(Vec3(0, 0, ts*(y+1)) + offset, Vec3(-1, 0, 0), Vec2(0, 0)));
				v.push_back(Vertex(Vec3(0, 0, ts*y) + offset, Vec3(-1, 0, 0), Vec2(uv.x, 0)));
			}
			// right
			if(!building->IsDoors(Int2(size.x - 1, y), DIR_RIGHT))
			{
				v.push_back(Vertex(Vec3(ts*size.x, h, ts*y) + offset, Vec3(-1, 0, 0), Vec2(0, uv.y)));
				v.push_back(Vertex(Vec3(ts*size.x, h, ts*(y+1)) + offset, Vec3(-1, 0, 0), Vec2(uv.x, uv.y)));
				v.push_back(Vertex(Vec3(ts*size.x, 0, ts*y) + offset, Vec3(-1, 0, 0), Vec2(0, 0)));
				v.push_back(Vertex(Vec3(ts*size.x, 0, ts*(y+1)) + offset, Vec3(-1, 0, 0), Vec2(uv.x, 0)));
			}
		}
		info.subs[1].tris = v.size() / 2 - info.subs[0].tris;
		// outside wall from inside
		info.subs[2].first = v.size() / 2;
		// ...
		info.subs[2].tris = v.size() / 2 - info.subs[1].tris;


		info.GenerateIndices();
		building->mesh = res_mgr->CreateMesh(&info);
		building->mesh->head.radius = (Vec3(0, h, ts*max(size.x, size.y)) + offset).Length(); // FIXME - use roof pos
	}
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
	for(Building* p_b : buildings)
	{
		Building& b = *p_b;
		SceneNode* building_node = new SceneNode;
		building_node->container = new SceneNode::Container;
		building_node->container->box = Box::CreateXZ(b.box, 0.f, 4.f);
		building_node->container->is_sphere = false;

		SceneNode* building_mesh = new SceneNode;
		building_mesh->mesh = b.mesh;
		building_mesh->pos = Vec3(tile_size * b.size.x / 2 + tile_size * b.pos.x, 0, tile_size * b.size.y / 2 + tile_size * b.pos.y);
		building_mesh->rot = Vec3::Zero;
		building_node->Add(building_mesh);

		Int2 size = b.size * 2;
		Int2 pos = b.pos * 2;

		/*for(int x = 1; x < size.x - 1; ++x)
		{
			// bottom wall
			if(!b.IsDoors(Int2(x, 0), DIR_BOTTOM))
			{
				node = new SceneNode;
				node->mesh = mesh_wall;
				node->pos = Vec3(tile_size2 * (x + pos.x + 1), 0, tile_size2 * pos.y);
				node->rot = Vec3(0, PI / 2, 0);
				building_node->Add(node);
			}

			// top wall
			if(!b.IsDoors(Int2(x, size.y - 1), DIR_TOP))
			{
				node = new SceneNode;
				node->mesh = mesh_wall;
				node->pos = Vec3(tile_size2 * (x + pos.x + 1), 0, tile_size2 * (pos.y + size.y) - wall_width);
				node->rot = Vec3(0, PI / 2, 0);
				building_node->Add(node);
			}
		}

		for(int y = 1; y < size.y - 1; ++y)
		{
			// left wall
			if(!b.IsDoors(Int2(0, y), DIR_LEFT))
			{
				node = new SceneNode;
				node->mesh = mesh_wall;
				node->pos = Vec3(tile_size2 * pos.x, 0, tile_size2 * (y + pos.y));
				node->rot = Vec3::Zero;
				building_node->Add(node);
			}

			// right wall
			if(!b.IsDoors(Int2(size.x - 1, y), DIR_RIGHT))
			{
				node = new SceneNode;
				node->mesh = mesh_wall;
				node->pos = Vec3(tile_size2 * (pos.x + size.x) - wall_width, 0, tile_size2 * (y + pos.y));
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
		building_node->Add(node);*/

		scene->Add(building_node);

		// colliders
		/*Collider c;
		c.half_size = Vec2(tile_size2 / 2, wall_width / 2);
		for(int x = 0; x < size.x; ++x)
		{
			// bottom wall
			c.center = Vec2(tile_size2 * (x + pos.x) + tile_size2 / 2, tile_size2 * pos.y + wall_width / 2);
			if(!b.IsDoors(Int2(x, 0), DIR_BOTTOM))
				level->AddCollider(c);

			// top wall
			if(!b.IsDoors(Int2(x, size.y - 1), DIR_TOP))
			{
				c.center.y = tile_size2 * (pos.y + size.y) - wall_width / 2;
				level->AddCollider(c);
			}
		}
		c.half_size = Vec2(wall_width / 2, tile_size2 / 2);
		for(int y = 0; y < size.y; ++y)
		{
			// left wall
			c.center = Vec2(tile_size2 * pos.x + wall_width / 2, tile_size2 * (y + pos.y) + tile_size2 / 2);
			if(!b.IsDoors(Int2(0, y), DIR_LEFT))
				level->AddCollider(c);

			// right wall
			if(!b.IsDoors(Int2(size.x - 1, y), DIR_RIGHT))
			{
				c.center.x = tile_size2 * (pos.x + size.x) - wall_width / 2;
				level->AddCollider(c);
			}
		}*/

		level->camera_colliders.push_back(Box(tile_size2 * pos.x, 4.f, tile_size2 * pos.y,
			tile_size2 * (pos.x + size.x), 4.05f, tile_size2 * (pos.y + size.y)));
	}
}

void CityGenerator::SpawnItems()
{
	Item* medkit = Item::Get("medkit");
	Item* food = Item::Get("canned_food");
	Item* ammo = Item::Get("pistol_ammo");
	Item* pistol = Item::Get("pistol");
	Item* axe = Item::Get("axe");

	for(Building* building : buildings)
	{
		int count = Rand() % 10; // 10% - 0, 40% - 1, 40% - 2, 10% - 3
		switch(count)
		{
		case 0:
			break;
		case 1:
		case 2:
		case 3:
		case 4:
			count = 1;
			break;
		case 5:
		case 6:
		case 7:
		case 8:
			count = 2;
			break;
		case 9:
			count = 3;
			break;
		}

		for(int i = 0; i < count; ++i)
		{
			Item* item;
			switch(Rand() % 4)
			{
			default:
			case 0:
				item = medkit;
				break;
			case 1:
				item = food;
				break;
			case 2:
				item = ammo;
				break;
			case 3:
				item = pistol;
				break;
			}

			Vec2 pos = building->box.GetRandomPoint(2.f);
			level->SpawnItem(Vec3(pos.x, floor_y, pos.y), item);
		}
	}

	for(int i = 0; i < 2; ++i)
	{
		Building* building = buildings[Rand() % buildings.size()];
		Vec2 pos = building->box.GetRandomPoint(2.f);
		level->SpawnItem(Vec3(pos.x, floor_y, pos.y), axe);
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

void CityGenerator::Save(FileWriter& f)
{
	f << map;
	f << buildings; // FIXME
}

void CityGenerator::Load(FileReader& f)
{
	f >> map;
	f >> buildings;
	CreateScene();
	level->SpawnBarriers();
	pathfinding->GenerateBlockedGrid(size, tile_size, buildings);
}
