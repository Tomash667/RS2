#include "GameCore.h"
#include "Pathfinding.h"
#include <DebugDrawer.h>
#include "Level.h"

void Pathfinding::GenerateBlockedGrid(uint size, float tile_size, const vector<Building>& buildings)
{
	uint s = size * 2;
	this->size = s;
	this->tile_size = tile_size / 2;
	calculation_id = 0;

	tiles.resize(s * s);
	memset(tiles.data(), 0, sizeof(Tile) * s * s);

	for(const Building& b : buildings)
	{
		Int2 pos = b.pos * 2,
			size = b.size * 2;
		for(int x = pos.x; x < pos.x + size.x; ++x)
		{
			// bottom
			if(x != b.doors[Building::DOOR_BOTTOM])
			{
				tiles[x + pos.y * s].blocked |= BLOCKED_BOTTOM;
				tiles[x + (pos.y - 1) * s].blocked |= BLOCKED_TOP;
			}
			// top
			if(x != b.doors[Building::DOOR_TOP])
			{
				tiles[x + (pos.y + size.y - 1) * s].blocked |= BLOCKED_TOP;
				tiles[x + (pos.y + size.y) * s].blocked |= BLOCKED_BOTTOM;
			}
		}
		for(int y = pos.y; y < pos.y + size.y; ++y)
		{
			// left
			if(y != b.doors[Building::DOOR_LEFT])
			{
				tiles[pos.x + y * s].blocked |= BLOCKED_LEFT;
				tiles[pos.x - 1 + y * s].blocked |= BLOCKED_RIGHT;
			}
			// right
			if(y != b.doors[Building::DOOR_RIGHT])
			{
				tiles[pos.x + size.x - 1 + y * s].blocked |= BLOCKED_RIGHT;
				tiles[pos.x + size.x + y * s].blocked |= BLOCKED_LEFT;
			}
		}
	}
}

void Pathfinding::DrawPath(DebugDrawer* debug_drawer, const Vec3& from, const Vec3& to, const vector<Int2>& path)
{
	debug_drawer->SetColor(Color(0, 0, 0, 128));
	debug_drawer->DrawSphere(from + Vec3(0, 0.5f, 0), 0.15f);

	debug_drawer->SetColor(Color(255, 255, 255, 128));
	debug_drawer->DrawSphere(to + Vec3(0, 0.5f, 0), 0.15f);

	if(!path.empty())
	{
		Color c[6] = {
			Color(255,0,0),
			Color(0,255,0),
			Color(0,0,255),
			Color(255,255,0),
			Color(0,255,255),
			Color(255,0,255)
		};
		int c_index = 0;

		const float width = 0.02f;
		debug_drawer->SetColor(c[c_index]);
		c_index = (c_index + 1) % 6;
		Int2 start = path.front();
		debug_drawer->DrawLine(from + Vec3(0, 0.5f, 0), Vec3(tile_size * start.x + tile_size / 2, 0.5f, tile_size * start.y + tile_size / 2), width);
		for(uint i = 1, count = path.size(); i < count; ++i)
		{
			debug_drawer->SetColor(c[c_index]);
			c_index = (c_index + 1) % 6;
			Int2 prev_pt = path[i - 1];
			Int2 pt = path[i];
			debug_drawer->DrawLine(Vec3(tile_size * prev_pt.x + tile_size / 2, 0.5f, tile_size * prev_pt.y + tile_size / 2),
				Vec3(tile_size * pt.x + tile_size / 2, 0.5f, tile_size * pt.y + tile_size / 2), width);
		}
		debug_drawer->SetColor(c[c_index]);
		c_index = (c_index + 1) % 6;
		Int2 end = path.back();
		debug_drawer->DrawLine(Vec3(tile_size * end.x + tile_size / 2, 0.5f, tile_size * end.y + tile_size / 2), to + Vec3(0, 0.5f, 0), width);
	}
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
	Tile& tile = tiles[start_pt.x + start_pt.y * size];
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
		Tile& current_tile = tiles[pt.x + pt.y * size];

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
			Tile& new_tile = tiles[new_pt.x + new_pt.y * size];
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
				const Tile& bt1 = tiles[pt1.x + pt1.y * size];
				const Tile& bt2 = tiles[pt2.x + pt2.y * size];
				return bt1.total_cost > bt2.total_cost;
			});
		}
	}

	if(done)
	{
		Int2 pt = target_pt;
		tmp_path.clear();
		while(pt != start_pt)
		{
			tmp_path.push_back(pt);
			pt = tiles[pt.x + pt.y * size].prev;
		}
		tmp_path.push_back(start_pt);
		std::reverse(tmp_path.begin(), tmp_path.end());
		SimplifyPath(tmp_path, path);
		return FPR_FOUND;
	}
	else
		return FPR_NOT_FOUND;
}

void Pathfinding::SimplifyPath(vector<Int2>& path, vector<Int2>& results)
{
	if(path.size() <= 2u)
	{
		results = path;
		return;
	}
	Int2 prev = path.front();
	results.push_back(prev);
	for(uint i = 0, count = path.size() - 2; i < count; ++i)
	{
		// if there is line of sight from prev to P(i+2) then P(i+1) can be removed
		if(!LineTest(prev, path[i + 2]))
		{
			results.push_back(path[i + 1]);
			prev = path[i + 1];
		}
	}
	results.push_back(path.back());
}

bool Pathfinding::LineTest(const Int2& pt1, const Int2& pt2)
{
	if(pt1.x == pt2.x)
	{
		// y test
		if(pt1.y < pt2.y)
		{
			for(int y = pt1.y; y < pt2.y; ++y)
			{
				if(IS_SET(tiles[pt1.x + y * size].blocked, BLOCKED_BOTTOM))
					return false;
			}
		}
		else
		{
			for(int y = pt2.y; y > pt1.y; ++y)
			{
				if(IS_SET(tiles[pt1.x + y * size].blocked, BLOCKED_TOP))
					return false;
			}
		}
	}
	else if(pt1.y == pt2.y)
	{
		// x test
		if(pt1.x < pt2.x)
		{
			for(int x = pt1.x; x < pt2.x; ++x)
			{
				if(IS_SET(tiles[x + pt1.y * size].blocked, BLOCKED_RIGHT))
					return false;
			}
		}
		else
		{
			for(int x = pt2.x; x > pt1.x; ++x)
			{
				if(IS_SET(tiles[x + pt1.y * size].blocked, BLOCKED_LEFT))
					return false;
			}
		}
	}
	else
	{
		// complex test...
		int check_blocked;
		if(pt1.x < pt2.x)
		{
			if(pt1.y < pt2.y)
				check_blocked = BLOCKED_RIGHT | BLOCKED_TOP;
			else
				check_blocked = BLOCKED_RIGHT | BLOCKED_BOTTOM;
		}
		else
		{
			if(pt1.y < pt2.y)
				check_blocked = BLOCKED_LEFT | BLOCKED_TOP;
			else
				check_blocked = BLOCKED_LEFT | BLOCKED_BOTTOM;
		}
		Int2 min, max;
		Int2::MinMax(pt1, pt2, min, max);
		for(int y = min.y; y <= max.y; ++y)
		{
			for(int x = min.x; x <= max.x; ++x)
			{
				if(IS_SET(tiles[x + y * size].blocked, check_blocked))
					return false;
			}
		}
	}

	return true;
}

Vec3 Pathfinding::GetPathNextTarget(const Vec3& target, const vector<Int2>& path)
{
	if(path.size() <= 2u)
		return target;
	else
		return PtToPos(path[1]);
}
