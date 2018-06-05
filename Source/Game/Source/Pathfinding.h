#pragma once

#include "Building.h"

class Pathfinding
{
public:
	enum FindPathResult
	{
		FPR_OUTSIDE,
		FPR_SAME_TILE,
		FPR_NOT_FOUND,
		FPR_FOUND
	};

	void GenerateBlockedGrid(uint size, float tile_size, const vector<Building>& buildings);
	void DrawPath(DebugDrawer* debug_drawer, const Vec3& from, const Vec3& to, const vector<Int2>& path);
	FindPathResult FindPath(const Vec3& from, const Vec3& to, vector<Int2>& path);
	Int2 GetPt(const Vec3& pos) { return Int2(int(pos.x / tile_size), int(pos.z / tile_size)); }
	Vec3 PtToPos(const Int2& pt) { return Vec3(tile_size * pt.x + tile_size / 2, 0, tile_size * pt.y + tile_size / 2); }
	Vec3 GetPathNextTarget(const Vec3& target, const vector<Int2>& path);

private:
	enum Blocked
	{
		BLOCKED_LEFT = 1 << 0,
		BLOCKED_RIGHT = 1 << 1,
		BLOCKED_TOP = 1 << 2,
		BLOCKED_BOTTOM = 1 << 3
	};

	struct Tile
	{
		int blocked, cost, dist, total_cost, calculation_id;
		Int2 prev;
	};

	void SimplifyPath(vector<Int2>& path, vector<Int2>& results);
	bool LineTest(const Int2& pt1, const Int2& pt2);

	Level* level;
	vector<Tile> tiles;
	uint size;
	float tile_size;
	vector<Collider> colliders;
	Int2 last_pos;
	int calculation_id;
	vector<Int2> to_check, tmp_path;
};
