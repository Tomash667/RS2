#pragma once

class Navmesh
{
public:
	void Reset();
	void StartRegion(const Box2d& box);
	//void EndRegion();
	//void AddCollier();
	void Draw(DebugDrawer* debug_drawer);

private:
	vector<Box2d> regions;
};
