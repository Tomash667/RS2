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

	void Init(Level* level);
	void GenerateBlockedGrid(uint size, float tile_size, const vector<Building>& buildings);
	void FillCollisionGrid(const Vec3& pos);
	void Draw(DebugDrawer* debug_drawer);
	FindPathResult FindPath(const Vec3& from, const Vec3& to, vector<Int2>& path);
	
private:
	enum Blocked
	{
		BLOCKED_LEFT = 1 << 0,
		BLOCKED_RIGHT = 1 << 1,
		BLOCKED_TOP = 1 << 2,
		BLOCKED_BOTTOM = 1 << 3
	};

	struct BigTile
	{
		int blocked, cost, dist, total_cost, calculation_id;
		Int2 prev;
	};

	struct Tile
	{
		bool blocked;
	};

	bool Collide(Box2d& box);

	Level* level;
	vector<BigTile> big_tiles;
	vector<Tile> tiles;
	uint size;
	float tile_size;
	vector<Collider> colliders;
	Int2 last_pos;
	int calculation_id;
	vector<Int2> to_check;
};
