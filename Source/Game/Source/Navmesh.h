#pragma once

class Navmesh
{
public:
	void Reset();
	void StartRegion(const Box2d& box) { region = box; }
	void EndRegion();
	void AddCollier(const Box2d& box) { colliders.push_back(box); }
	void Draw(DebugDrawer* debug_drawer);

private:
	struct Triangle
	{
		Vec2 pos[3];
	};
	vector<Triangle> triangles;
	vector<Box2d> regions, colliders;
	Box2d region;
};
