#include "GameCore.h"
#include "CityGenerator.h"
#include <Scene.h>
#include <SceneNode.h>
#include <ResourceManager.h>
#include <ScenePart.h>
#include "Level.h"
#include "Player.h"
#include "Item.h"
#include "Tree.h"
#include <MeshBuilder.h>
#include "Navmesh.h"

const float CityGenerator::tile_size = 5.f;
const float CityGenerator::floor_y = 0.05f;
const float CityGenerator::wall_width = 0.2f;
const float ts = CityGenerator::tile_size / 2;
const float jamb_size = 0.25f;
const uint NAVMESH_TILES = 32;

CityGenerator::CityGenerator() : navmesh_timer(false)
{
}

CityGenerator::~CityGenerator()
{
	DeleteElements(buildings);
}

void CityGenerator::Init(Scene* scene, Level* level, ResourceManager* res_mgr, uint size, uint splits, Navmesh* navmesh)
{
	this->res_mgr = res_mgr;
	this->scene = scene;
	this->level = level;
	this->size = size;
	this->navmesh = navmesh;

	scene->InitQuadTree(tile_size * size, splits);
	map.resize(size * size);
	map_size = tile_size * size;

	mesh[T_ASPHALT] = res_mgr->GetMesh("buildings/asphalt.qmsh");
	mesh[T_PAVEMENT] = res_mgr->GetMesh("buildings/brick_pavement.qmsh");
	mesh[T_BUILDING] = res_mgr->GetMesh("buildings/building_floor.qmsh");

	mesh_offset[T_ASPHALT] = 0;
	mesh_offset[T_PAVEMENT] = floor_y;
	mesh_offset[T_BUILDING] = floor_y;

	mesh_curb = res_mgr->GetMesh("buildings/curb.qmsh");
	mesh_table = res_mgr->GetMesh("objects/table.qmsh");

	mesh_wall = res_mgr->GetMeshRaw("buildings/wall.qmsh");
	mesh_wall_inner = res_mgr->GetMeshRaw("buildings/wall_inner.qmsh");
	mesh_corner = res_mgr->GetMeshRaw("buildings/corner.qmsh");
	mesh_door_jamb = res_mgr->GetMeshRaw("buildings/door_jamb.qmsh");
	mesh_door_jamb_inner = res_mgr->GetMeshRaw("buildings/door_jamb_inner.qmsh");
	mesh_ceil = res_mgr->GetMeshRaw("buildings/ceil.qmsh");

	navmesh_thread_state = THREAD_NOT_STARTED;
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
	BuildBuildingsMesh();
	CreateScene();
	BuildNavmesh();
	level->SpawnBarriers();
	level->SpawnPlayer(player_start_pos);
	SpawnItems();
	SpawnZombies();
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
		if(node->IsLeaf())
			continue;
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

			// top
			if(room.pos.y != 0)
			{
				for(uint x = room.pos.x, count = room.pos.x + room.size.x; x < count; ++x)
					building->CheckConnect(room, indices[x + (room.pos.y - 1) * size.x], last);
			}
			else
				room.outside |= DIR_F_TOP;

			// bottom
			if(room.pos.y + room.size.y != size.y)
			{
				for(uint x = room.pos.x, count = room.pos.x + room.size.x; x < count; ++x)
					building->CheckConnect(room, indices[x + (room.pos.y + room.size.y) * size.x], last);
			}
			else
				room.outside |= DIR_F_BOTTOM;

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
					case DIR_TOP:
						rect = Rect(room->pos.x, room->pos.y, room->pos.x + room->size.x - 1, room->pos.y);
						break;
					case DIR_BOTTOM:
						rect = Rect(room->pos.x, room->pos.y + room->size.y - 1, room->pos.x + room->size.x - 1, room->pos.y + room->size.y - 1);
						break;
					}

					Int2 pt;
					if(rect.p1.x == rect.p2.x)
					{
						// left or right
						pt.x = rect.p1.x;
						pt.y = Random(rect.p1.y, rect.p2.y);
						if(pt.y == rect.p1.y || pt.y == rect.p2.y)
							pt.y = Random(rect.p1.y, rect.p2.y);
						if(pt.x == 0 || pt.x == size.x - 1)
						{
							if(pt.y == 0)
								++pt.y;
							else if(pt.y == size.y - 1)
								--pt.y;
						}
					}
					else
					{
						// top or bottom
						pt.y = rect.p1.y;
						pt.x = Random(rect.p1.x, rect.p2.x);
						if(pt.x == rect.p1.x || pt.x == rect.p2.x)
							pt.x = Random(rect.p1.x, rect.p2.x);
						if(pt.y == 0 || pt.y == size.y - 1)
						{
							if(pt.x == 0)
								++pt.x;
							else if(pt.x == size.x - 1)
								--pt.x;
						}
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
					uint other_index = room.connected[index];
					Building::Room& room2 = building->rooms[other_index];
					if(room2.visited)
					{
						room.visited = true;
						if(!room2.IsConnected(my_index))
						{
							room.connected2.push_back(other_index);
							room2.connected2.push_back(my_index);
						}
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

		// add inside doors
		for(uint index = 0; index < building->rooms.size(); ++index)
		{
			Building::Room& room = building->rooms[index];
			for(uint index2 : room.connected2)
			{
				if(index > index2)
					continue;
				Building::Room& room2 = building->rooms[index2];
				Rect intersection;
				bool ok = Rect::Intersect(Rect::Create(room.pos, room.size), Rect::Create(room2.pos, room2.size), intersection);
				assert(ok);
				Int2 pt;
				DIR dir;
				if(intersection.p1.x == intersection.p2.x)
				{
					--intersection.p2.y;
					dir = DIR_LEFT;
					pt.x = intersection.p1.x;
					pt.y = RandomNormal(intersection.p1.y, intersection.p2.y);
				}
				else
				{
					--intersection.p2.x;
					dir = DIR_TOP;
					pt.y = intersection.p1.y;
					pt.x = RandomNormal(intersection.p1.x, intersection.p2.x);
				}
				building->doors.push_back(std::make_pair(pt, dir));
			}
		}

		// spawn tables
		for(Building::Room& room : building->rooms)
		{
			if(Rand() % 4 == 0)
			{
				Int2 pt = Int2(RandomNormal(1, room.size.x - 2), RandomNormal(1, room.size.y - 2)) + room.pos;
				bool rotated = Rand() % 2 == 0;
				building->tables.push_back({ pt, rotated });
			}
		}
	}
}

void CityGenerator::BuildBuildingsMesh()
{
	MeshBuilder builder;
	Matrix rotPI_2 = Matrix::RotationY(PI / 2),
		rotPI = Matrix::RotationY(PI),
		rotPI3_2 = Matrix::RotationY(PI * 3 / 2);

	for(Building* building : buildings)
	{
		const Int2 size = building->size * 2;
		building->SetIsDoorMap();

		// build mesh
		builder.Clear();
		Vec3 offset = Vec3(-tile_size * building->size.x / 2, 0, -tile_size * building->size.y / 2);
		Vec2 col_offset = Vec2(building->pos.x * tile_size, building->pos.y * tile_size);
		const float ts2 = ts / 2;
		const float wall_width2 = wall_width / 2;
		const float jamb_size2 = 0.25f / 2;
		const Vec2 door_jamb_y(2.5f, 4.f - 2.5f);
		Vec3 cam_offset = Vec3(col_offset.x, door_jamb_y.x, col_offset.y);
		// corners
		//   left top
		builder.Append(mesh_corner, Matrix::Translation(offset));
		level->AddCollider(Collider(Vec2(wall_width2, ts2) + col_offset, Vec2(wall_width2, ts2)));
		level->AddCollider(Collider(Vec2(ts2, wall_width2) + col_offset, Vec2(ts2, wall_width2)));
		//   right top
		builder.Append(mesh_corner, rotPI3_2 * Matrix::Translation(offset + Vec3(building->size.x * tile_size, 0, 0)));
		level->AddCollider(Collider(Vec2(building->size.x * tile_size - wall_width2, ts2) + col_offset, Vec2(wall_width2, ts2)));
		level->AddCollider(Collider(Vec2(building->size.x * tile_size - ts2, wall_width2) + col_offset, Vec2(ts2, wall_width2)));
		//   left bottom
		builder.Append(mesh_corner, rotPI_2 * Matrix::Translation(offset + Vec3(0, 0, building->size.y * tile_size)));
		level->AddCollider(Collider(Vec2(wall_width2, building->size.y * tile_size - ts2) + col_offset, Vec2(wall_width2, ts2)));
		level->AddCollider(Collider(Vec2(ts2, building->size.y * tile_size - wall_width2) + col_offset, Vec2(ts2, wall_width2)));
		//   right bottom
		builder.Append(mesh_corner, rotPI * Matrix::Translation(offset + Vec3(building->size.x * tile_size, 0, building->size.y * tile_size)));
		level->AddCollider(Collider(Vec2(building->size.x * tile_size - wall_width2, building->size.y * tile_size - ts2) + col_offset, Vec2(wall_width2, ts2)));
		level->AddCollider(Collider(Vec2(building->size.x * tile_size - ts2, building->size.y * tile_size - wall_width2) + col_offset, Vec2(ts2, wall_width2)));
		// outside walls
		for(int x = 1; x < size.x - 1; ++x)
		{
			// top
			bool is_door = building->IsDoor(Int2(x, 0), DIR_TOP);
			builder.Append(is_door ? mesh_door_jamb : mesh_wall,
				rotPI3_2 * Matrix::Translation(offset + Vec3(x*ts + ts2, 0, wall_width2)));
			if(is_door)
			{
				level->AddCollider(Collider(Vec2(x * ts + jamb_size2, wall_width2) + col_offset, Vec2(jamb_size2, wall_width2)));
				level->AddCollider(Collider(Vec2((x + 1) * ts - jamb_size2, wall_width2) + col_offset, Vec2(jamb_size2, wall_width2)));
				level->camera_colliders.push_back(Box::Create(Vec3(x * ts, 0, 0) + cam_offset, Vec3(ts, door_jamb_y.y, wall_width)));
			}
			else
				level->AddCollider(Collider(Vec2(x * ts + ts2, wall_width2) + col_offset, Vec2(ts2, wall_width2)));
			// bottom
			is_door = building->IsDoor(Int2(x, size.y - 1), DIR_BOTTOM);
			builder.Append(is_door ? mesh_door_jamb : mesh_wall,
				rotPI_2 * Matrix::Translation(offset + Vec3(x*ts + ts2, 0, size.y * ts - wall_width2)));
			if(is_door)
			{
				level->AddCollider(Collider(Vec2(x * ts + jamb_size2, building->size.y * tile_size - wall_width2) + col_offset, Vec2(jamb_size2, wall_width2)));
				level->AddCollider(Collider(Vec2((x + 1) * ts - jamb_size2, building->size.y * tile_size - wall_width2) + col_offset, Vec2(jamb_size2, wall_width2)));
				level->camera_colliders.push_back(Box::Create(Vec3(x * ts, 0, building->size.y * tile_size - wall_width) + cam_offset, Vec3(ts, door_jamb_y.y, wall_width)));
			}
			else
				level->AddCollider(Collider(Vec2(x * ts + ts2, building->size.y * tile_size - wall_width2) + col_offset, Vec2(ts2, wall_width2)));
		}
		for(int y = 1; y < size.y - 1; ++y)
		{
			// left
			bool is_door = building->IsDoor(Int2(0, y), DIR_LEFT);
			builder.Append(is_door ? mesh_door_jamb : mesh_wall,
				Matrix::Translation(offset + Vec3(wall_width2, 0, y * ts + ts2)));
			if(is_door)
			{
				level->AddCollider(Collider(Vec2(wall_width2, y * ts + jamb_size2) + col_offset, Vec2(wall_width2, jamb_size2)));
				level->AddCollider(Collider(Vec2(wall_width2, (y + 1) * ts - jamb_size2) + col_offset, Vec2(wall_width2, jamb_size2)));
				level->camera_colliders.push_back(Box::Create(Vec3(0, 0, y * ts) + cam_offset, Vec3(wall_width, door_jamb_y.y, ts)));
			}
			else
				level->AddCollider(Collider(Vec2(wall_width2, y * ts + ts2) + col_offset, Vec2(wall_width2, ts2)));
			// right
			is_door = building->IsDoor(Int2(size.x - 1, y), DIR_RIGHT);
			builder.Append(is_door ? mesh_door_jamb : mesh_wall,
				rotPI * Matrix::Translation(offset + Vec3(size.x * ts - wall_width2, 0, y * ts + ts2)));
			if(is_door)
			{
				level->AddCollider(Collider(Vec2(building->size.x * tile_size - wall_width2, y * ts + jamb_size2) + col_offset, Vec2(wall_width2, jamb_size2)));
				level->AddCollider(Collider(Vec2(building->size.x * tile_size - wall_width2, (y + 1) * ts - jamb_size2) + col_offset, Vec2(wall_width2, jamb_size2)));
				level->camera_colliders.push_back(Box::Create(Vec3(building->size.x * tile_size - wall_width, 0, y * ts) + cam_offset, Vec3(wall_width, door_jamb_y.y, ts)));
			}
			else
				level->AddCollider(Collider(Vec2(building->size.x * tile_size - wall_width2, y * ts + ts2) + col_offset, Vec2(wall_width2, ts2)));
		}
		// inner walls
		for(Building::Room& room : building->rooms)
		{
			// right
			if(!IS_SET(room.outside, DIR_F_RIGHT))
			{
				int x = room.pos.x + room.size.x;
				for(int y = room.pos.y; y < room.pos.y + room.size.y; ++y)
				{
					bool is_door = building->IsDoor(Int2(x, y), DIR_LEFT);
					builder.Append(is_door ? mesh_door_jamb_inner : mesh_wall_inner,
						Matrix::Translation(offset + Vec3(x * ts, 0, y * ts + ts2)));
					if(is_door)
					{
						level->AddCollider(Collider(Vec2(x * ts, y * ts + jamb_size2) + col_offset, Vec2(wall_width2, jamb_size2)));
						level->AddCollider(Collider(Vec2(x * ts, (y + 1) * ts - jamb_size2) + col_offset, Vec2(wall_width2, jamb_size2)));
						level->camera_colliders.push_back(Box::Create(Vec3(x * ts, 0, y * ts) + cam_offset, Vec3(wall_width, door_jamb_y.y, ts)));
					}
					else
						level->AddCollider(Collider(Vec2(x * ts, y * ts + ts2) + col_offset, Vec2(wall_width2, ts2)));
				}
			}
			// bottom
			if(!IS_SET(room.outside, DIR_F_BOTTOM))
			{
				int y = room.pos.y + room.size.y;
				for(int x = room.pos.x; x < room.pos.x + room.size.x; ++x)
				{
					bool is_door = building->IsDoor(Int2(x, y), DIR_TOP);
					builder.Append(is_door ? mesh_door_jamb_inner : mesh_wall_inner,
						rotPI_2 * Matrix::Translation(offset + Vec3(x*ts + ts2, 0, y*ts)));
					if(is_door)
					{
						level->AddCollider(Collider(Vec2(x * ts + jamb_size2, y * ts) + col_offset, Vec2(jamb_size2, wall_width2)));
						level->AddCollider(Collider(Vec2((x + 1) * ts - jamb_size2, y * ts) + col_offset, Vec2(jamb_size2, wall_width2)));
						level->camera_colliders.push_back(Box::Create(Vec3(x * ts, 0, y * ts) + cam_offset, Vec3(ts, door_jamb_y.y, wall_width)));
					}
					else
						level->AddCollider(Collider(Vec2(x * ts + ts2, y * ts) + col_offset, Vec2(ts2, wall_width2)));
				}
			}
		}
		// ceil
		for(int y = 0; y < building->size.y; ++y)
		{
			for(int x = 0; x < building->size.x; ++x)
				builder.Append(mesh_ceil, Matrix::Translation(offset + Vec3(tile_size * x, 0, tile_size * y)));
		}
		level->camera_colliders.push_back(Box(tile_size * building->pos.x, 4.f, tile_size * building->pos.y,
			tile_size * (building->pos.x + building->size.x), 4.05f, tile_size * (building->pos.y + building->size.y)));
		// finalize
		builder.JoinIndices();
		building->mesh = res_mgr->CreateMesh(&builder);
		building->mesh->head.radius = (Vec3(0, 4.1f, (tile_size / 2)*max(size.x, size.y)) + offset).Length();
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
					// top
					node = new SceneNode;
					node->pos = Vec3(tile_size * x + tile_size2, 0, tile_size * y);
					node->rot = Vec3(0, PI / 2, 0);
					node->mesh = mesh_curb;
					part->Add(node);
				}
				if(y < size - 1 && map[x + (y + 1) * size] != T_ASPHALT)
				{
					// botom
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

		for(Building::Table& table : b.tables)
		{
			SceneNode* node = new SceneNode;
			node->mesh = mesh_table;
			node->pos = Vec3(tile_size * b.pos.x + tile_size / 2 * table.pos.x + tile_size / 4,
				0, tile_size * b.pos.y + tile_size / 2 * table.pos.y + tile_size / 4);
			Vec2 half_ext = Vec2(1.9f / 2, 1.1f / 2);
			if(table.rotated)
			{
				node->rot = Vec3(0, PI / 2, 0);
				half_ext.Swap();
			}
			else
				node->rot = Vec3::Zero;
			building_node->Add(node);
			level->AddCollider(Collider(node->pos.XZ(), half_ext, false));
		}

		scene->Add(building_node);
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

			SpawnItem(building, item);
		}
	}

	for(int i = 0; i < 2; ++i)
	{
		Building* building = buildings[Rand() % buildings.size()];
		SpawnItem(building, axe);
	}
}

void CityGenerator::SpawnItem(Building* building, Item* item)
{
	Building::Room& room = building->rooms[Rand() % building->rooms.size()];
	while(true)
	{
		Vec2 pos = Vec2(Random(wall_width * 2, room.size.x * ts - wall_width * 2), Random(wall_width * 2, room.size.y * ts - wall_width * 2));
		Int2 pt = Int2(int(pos.x / ts) + room.pos.x, int(pos.y / ts) + room.pos.y);
		bool ok = true;
		for(Building::Table& table : building->tables)
		{
			if(pt == table.pos)
			{
				ok = false;
				break;
			}
		}
		if(ok)
		{
			level->SpawnItem(Vec3(pos.x + room.pos.x * ts + building->pos.x * tile_size, floor_y, pos.y + room.pos.y * ts + building->pos.y * tile_size), item);
			return;
		}
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
	f << buildings.size();
	for(Building* b : buildings)
		b->Save(f);

	navmesh->Save(f);
	bool done = navmesh_built != 0;
	f << done;
	if(!done)
		f << navmesh_next_tile;
}

void CityGenerator::Load(FileReader& f)
{
	f >> map;
	uint count;
	f >> count;
	buildings.reserve(count);
	for(uint i = 0; i < count; ++i)
	{
		Building* b = new Building;
		b->Load(f);
		buildings.push_back(b);
	}
	BuildBuildingsMesh();
	CreateScene();
	level->SpawnBarriers();

	navmesh->PrepareTiles(map_size / NAVMESH_TILES, NAVMESH_TILES);
	navmesh->Load(f);
	bool done;
	f >> done;
	if(!done)
	{
		f >> navmesh_next_tile;
		navmesh_timer.Start();
		navmesh_built = 0;
		navmesh_thread_state = THREAD_WORKING;
		navmesh_thread = std::thread(&CityGenerator::NavmeshThreadLoop, this);
	}
	else
		navmesh_built = 2;
}

void CityGenerator::NavmeshThreadLoop()
{
	while(true)
	{
		switch(navmesh_thread_state)
		{
		case THREAD_NOT_STARTED:
		case THREAD_FINISHED:
			assert(0);
			break;
		case THREAD_QUIT:
			return;
		case THREAD_WORKING:
			{
				BuildNavmeshTile(navmesh_next_tile, true);
				++navmesh_next_tile.x;
				if(navmesh_next_tile.x == NAVMESH_TILES)
				{
					navmesh_next_tile.x = 0;
					navmesh_next_tile.y++;
					if(navmesh_next_tile.y == NAVMESH_TILES)
					{
						navmesh_built = 1;
						navmesh_thread_state = THREAD_FINISHED;
						return;
					}
				}
			}
			break;
		}
	}
}

void CityGenerator::BuildNavmesh()
{
	// no tiles
	//BuildNavmeshTile(Int2::Zero, false);

	navmesh->PrepareTiles(map_size / NAVMESH_TILES, NAVMESH_TILES);

	// build blocking
	//const int tiles_to_build = NAVMESH_TILES;
	//for(int x = 0; x < tiles_to_build; ++x)
	//	for(int y = 0; y < tiles_to_build; ++y)
	//		BuildNavmeshTile(Int2(x, y), true);

	// build in background
	navmesh_timer.Start();
	navmesh_built = 0;
	navmesh_next_tile = Int2::Zero;
	navmesh_thread_state = THREAD_WORKING;
	navmesh_thread = std::thread(&CityGenerator::NavmeshThreadLoop, this);
}

void CityGenerator::CheckNavmeshGeneration()
{
	if(navmesh_built == 1)
	{
		navmesh_built = 2;
		float t = navmesh_timer.Tick();
		Info("Finished navmesh generation. Took %g sec.", t);
	}
}

void CityGenerator::WaitForNavmeshThread()
{
	if(navmesh_thread_state == THREAD_WORKING)
	{
		navmesh_thread_state = THREAD_QUIT;
		navmesh_thread.join();
	}
	else if(navmesh_thread_state == THREAD_FINISHED)
		navmesh_thread.join();
	navmesh_thread_state = THREAD_NOT_STARTED;
}

void CityGenerator::BuildNavmeshTile(const Int2& tile, bool is_tiled)
{
	geom.verts.clear();
	geom.tris.clear();

	// floor
	geom.verts.insert(geom.verts.end(),
		{
			Vec3(0, 0, 0),
			Vec3(map_size, 0, 0),
			Vec3(0, 0, map_size),
			Vec3(map_size, 0, map_size)
		}
	);
	geom.tris.insert(geom.tris.end(),
		{
			2,3,0,
			0,3,1
		}
	);

	// colliders
	Box2d box = is_tiled ? navmesh->GetBoxForTile(tile) : Box2d(0, 0, map_size, map_size);
	vector<Collider> colliders;
	level->GatherColliders(colliders, box);

	for(Collider& c : colliders)
	{
		Box box = c.ToBox();
		int offset = geom.verts.size();
		geom.verts.insert(geom.verts.end(),
			{
				box.v1,
				Vec3(box.v2.x, box.v1.y, box.v1.z),
				Vec3(box.v1.x, box.v1.y, box.v2.z),
				Vec3(box.v2.x, box.v1.y, box.v2.z),
				Vec3(box.v1.x, box.v2.y, box.v1.z),
				Vec3(box.v2.x, box.v2.y, box.v1.z),
				Vec3(box.v1.x, box.v2.y, box.v2.z),
				box.v2
			}
		);
#define TRI(a,b,c) offset+a,offset+b,offset+c
		geom.tris.insert(geom.tris.end(),
			{
				// left
				TRI(6,4,2),
				TRI(2,4,0),
				// right
				TRI(5,7,1),
				TRI(1,7,3),
				// top
				TRI(4,5,0),
				TRI(0,5,1),
				// bottom
				TRI(7,6,3),
				TRI(3,6,2)
			}
		);
#undef TRI
	}

	//geom.SaveObj("city.obj");

	NavmeshGeometry nav_geom;
	nav_geom.verts = geom.verts.data();
	nav_geom.vert_count = geom.verts.size();
	nav_geom.tris = geom.tris.data();
	nav_geom.tri_count = geom.tris.size() / 3;
	nav_geom.bounds = box.ToBoxXZ(0.f, 2.f);
	
	if(is_tiled)
		navmesh->BuildTile(tile, nav_geom);
	else
		navmesh->Build(nav_geom);
}

void LevelGeometry::SaveObj(cstring filename)
{
	TextWriter f(filename);
	f << "mtllib mat.mtl\no Plane\n";
	for(Vec3& v : verts)
		f << Format("v %g %g %g\n", v.x, v.y, v.z);
	for(uint i = 0; i < tris.size() / 3; ++i)
		f << Format("f %d %d %d\n", tris[i * 3 + 0] + 1, tris[i * 3 + 1] + 1, tris[i * 3 + 2] + 1);
}
