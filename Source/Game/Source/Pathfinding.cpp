#include "GameCore.h"
#include "Pathfinding.h"
#include <DebugDrawer.h>
#include "Level.h"
#include "Unit.h"

//const float tile_size = 0.3125f;

void Pathfinding::Init(Level* level)
{
	this->level = level;
	size = 32 + 1;
	tiles.resize(size * size);
}

void Pathfinding::GenerateBlockedGrid(uint big_size, float tile_size, const vector<Building>& buildings)
{
	uint s = size * 2;
	this->size = s;
	this->tile_size = tile_size / 2;
	calculation_id = 0;

	big_tiles.resize(s * s);
	memset(big_tiles.data(), 0, sizeof(BigTile) * s * s);

	for(const Building& b : buildings)
	{
		Int2 pos = b.pos * 2,
			size = b.size * 2;
		for(int x = pos.x; x < pos.x + size.x; ++x)
		{
			// bottom
			if(x != b.doors[Building::DOOR_BOTTOM])
			{
				big_tiles[x + pos.y * s].blocked |= BLOCKED_BOTTOM;
				big_tiles[x + (pos.y - 1) * s].blocked |= BLOCKED_TOP;
			}
			// top
			if(x != b.doors[Building::DOOR_TOP])
			{
				big_tiles[x + (pos.y + size.y - 1) * s].blocked |= BLOCKED_TOP;
				big_tiles[x + (pos.y + size.y) * s].blocked |= BLOCKED_BOTTOM;
			}
		}
		for(int y = pos.y; y < pos.y + size.y; ++y)
		{
			// left
			if(y != b.doors[Building::DOOR_LEFT])
			{
				big_tiles[pos.x + y * s].blocked |= BLOCKED_LEFT;
				big_tiles[pos.x - 1 + y * s].blocked |= BLOCKED_RIGHT;
			}
			// right
			if(y != b.doors[Building::DOOR_RIGHT])
			{
				big_tiles[pos.x + size.x - 1 + y * s].blocked |= BLOCKED_RIGHT;
				big_tiles[pos.x + size.x + y * s].blocked |= BLOCKED_LEFT;
			}
		}
	}
}

void Pathfinding::FillCollisionGrid(const Vec3& pos)
{
	/*
	const float radius = 0.3f;
	const int half_size = size / 2;
	Int2 tile_pos = Int2(pos.x / tile_size, pos.z / tile_size);
	memset(tiles.data(), 0, sizeof(Tile) * size * size);
	colliders.clear();
	level->GatherColliders(colliders, Box2d(pos.x - half_size * tile_size - radius, pos.z - half_size * tile_size - radius,
		pos.x + half_size * tile_size + radius, pos.z + half_size * tile_size + radius));
	for(uint y = 0; y < size; ++y)
	{
		for(uint x = 0; x < size; ++x)
		{
			Box2d box = Box2d(tile_size * (x + tile_pos.x - half_size) - radius, tile_size * (y + tile_pos.y - half_size) - radius);
			box.v2 += Vec2(radius * 2 + tile_size);
			if(Collide(box))
				tiles[x + y * size].blocked = true;
		}
	}
	last_pos = tile_pos;
	*/
}

void Pathfinding::Draw(DebugDrawer* debug_drawer)
{
	/*if(from_set)
	{
		debug_drawer->SetColor(Color(0, 0, 0, 128));
		debug_drawer->DrawSphere(from + Vec3(0, 0.5f, 0), 0.15f);
	}

	if(to_set)
	{
		debug_drawer->SetColor(Color(255, 255, 255, 128));
		debug_drawer->DrawSphere(to + Vec3(0, 0.5f, 0), 0.15f);
	}

	if(!path.empty())
	{
		const float width = 0.1f;
		debug_drawer->SetColor(Color(0, 0, 255));
		Int2 start = path.front();
		debug_drawer->DrawLine(from + Vec3(0, 0.5f, 0), Vec3(tile_size * start.x + tile_size / 2, 0.5f, tile_size * start.y + tile_size / 2), width);
		for(uint i = 1, count = path.size(); i < count; ++i)
		{
			Int2 prev_pt = path[i - 1];
			Int2 pt = path[i];
			debug_drawer->DrawLine(Vec3(tile_size * prev_pt.x + tile_size / 2, 0.5f, tile_size * prev_pt.y + tile_size / 2),
				Vec3(tile_size * pt.x + tile_size / 2, 0.5f, tile_size * pt.y + tile_size / 2), width);
		}
		Int2 end = path.back();
		debug_drawer->DrawLine(Vec3(tile_size * end.x + tile_size / 2, 0.5f, tile_size * end.y + tile_size / 2), to + Vec3(0, 0.5f, 0), width);
	}*/

	/*const int half_size = size / 2;
	debug_drawer->SetColor(Color(0, 0, 255, 128));
	Vec3 pos[4];
	for(int y = 0; y < size; ++y)
	{
		for(int x = 0; x < size; ++x)
		{
			if(!tiles[x + y * size].blocked)
			{
				pos[0] = Vec3(tile_size * (x + last_pos.x - half_size), 0.5f, tile_size * (y + last_pos.y - half_size));
				pos[1] = pos[0] + Vec3(tile_size, 0, 0);
				pos[2] = pos[0] + Vec3(0, 0, tile_size);
				pos[3] = pos[0] + Vec3(tile_size, 0, tile_size);
				debug_drawer->DrawQuad(pos);
			}
		}
	}*/
}

bool Pathfinding::Collide(Box2d& box)
{
	for(Collider& c : colliders)
	{
		if(RectangleToRectangle(box, c.ToBox2d()))
			return true;
	}
	return false;
}

Pathfinding::FindPathResult Pathfinding::FindPath(const Vec3& from, const Vec3& to, vector<Int2>& path)
{
	path.clear();
	Int2 start_pt = Int2(int(from.x / tile_size), int(from.z / tile_size));
	Int2 target_pt = Int2(int(to.x / tile_size), int(to.z / tile_size));
	if(start_pt == target_pt)
		return FPR_SAME_TILE;

	if(start_pt.x < 0 || start_pt.y < 0 || start_pt.x >= (int)size || start_pt.y >= (int)size
		|| target_pt.x < 0 || target_pt.y < 0 || target_pt.x >= (int)size || target_pt.y >= (int)size)
		return FPR_OUTSIDE;

	++calculation_id;
	BigTile& tile = big_tiles[start_pt.x + start_pt.y * size];
	tile.dist = Int2::Distance(start_pt, target_pt) * 10;
	tile.cost = 0;
	tile.total_cost = tile.dist;
	tile.calculation_id = calculation_id;
	to_check.clear();
	to_check.push_back(start_pt);

	bool done = false;
	while(!to_check.empty())
	{
		Int2 pt = to_check.back();
		to_check.pop_back();
		BigTile& current_tile = big_tiles[pt.x + pt.y * size];

		struct Dir
		{
			Int2 dir;
			int blocked_by, reverse_blocked, cost;
		};

		const Dir dirs[] = {
			{ Int2(-1, 0), BLOCKED_LEFT, 0, 10 },
			{ Int2(1, 0), BLOCKED_RIGHT, 0, 10 },
			{ Int2(0, -1), BLOCKED_BOTTOM, 0, 10 },
			{ Int2(0, 1), BLOCKED_TOP, 0, 10 },
			{ Int2(-1, -1), BLOCKED_LEFT | BLOCKED_BOTTOM, BLOCKED_RIGHT | BLOCKED_TOP, 15 },
			{ Int2(1, -1), BLOCKED_RIGHT | BLOCKED_BOTTOM, BLOCKED_LEFT | BLOCKED_TOP, 15 },
			{ Int2(-1, 1), BLOCKED_LEFT | BLOCKED_TOP, BLOCKED_RIGHT | BLOCKED_BOTTOM, 15 },
			{ Int2(1, 1), BLOCKED_RIGHT | BLOCKED_TOP, BLOCKED_LEFT | BLOCKED_BOTTOM, 15 },
		};

		bool added = false;
		for(int i = 0; i < 8; ++i)
		{
			const Dir& dir = dirs[i];
			if(IS_SET(current_tile.blocked, dir.blocked_by))
				continue;
			Int2 new_pt = pt + dir.dir;
			if(new_pt.x < 0 || new_pt.y < 0 || new_pt.x >= (int)size || new_pt.y >= (int)size)
				continue;
			BigTile& new_tile = big_tiles[new_pt.x + new_pt.y * size];
			if(IS_SET(new_tile.blocked, dir.reverse_blocked))
				continue;
			if(new_pt == target_pt)
			{
				new_tile.prev = pt;
				done = true;
				break;
			}
			int new_cost = current_tile.cost + dir.cost;
			if(new_tile.calculation_id != calculation_id)
			{
				new_tile.dist = Int2::Distance(new_pt, target_pt) * 10;
				new_tile.cost = new_cost;
				new_tile.total_cost = new_tile.dist + new_tile.cost;
				new_tile.calculation_id = calculation_id;
				new_tile.prev = pt;
				to_check.push_back(new_pt);
				added = true;
			}
			else if(new_cost < new_tile.cost)
			{
				new_tile.cost = new_cost;
				new_tile.total_cost = new_tile.cost + new_tile.dist;
				new_tile.prev = pt;
			}
		}
		
		if(done)
			break;

		if(added)
		{
			std::sort(to_check.begin(), to_check.end(), [&](const Int2& pt1, const Int2& pt2)
			{
				const BigTile& bt1 = big_tiles[pt1.x + pt1.y * size];
				const BigTile& bt2 = big_tiles[pt2.x + pt2.y * size];
				return bt1.total_cost > bt2.total_cost;
			});
		}
	}

	if(done)
	{
		Int2 pt = target_pt;
		path.push_back(pt);
		while(pt != start_pt)
		{
			pt = big_tiles[pt.x + pt.y * size].prev;
			path.push_back(pt);
		}
		std::reverse(path.begin(), path.end());
		return FPR_FOUND;
	}
	else
		return FPR_NOT_FOUND;
}
