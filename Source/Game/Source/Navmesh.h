#pragma once

// FIXME - hide implementation details
#include <Recast.h>
#include <DetourNavMesh.h>
#include <DetourNavMeshQuery.h>

class Navmesh
{
public:
	Navmesh();
	~Navmesh();
	bool Build(float level_size);
	//void StartRegion(const vector<Vec2>& outline) { this->outline = &outline; }
	//void EndRegion();
	//void AddCollier(const Box2d& box) { colliders.push_back(box); }
	//void AddPoint(const Vec2& pt) { points.push_back(pt); }
	//void Draw(DebugDrawer* debug_drawer);

	// FIXME
	float dt;
	Input* input;

private:
	void Reset();
	//void Triangulate(vector<p2t::Point*>* polyline, vector<p2t::Point*>* hole);

	//struct Triangle
	//{
	//	Vec2 pos[3];
	//};
	//vector<Triangle> triangles;

	//// current batch, don't save
	//const vector<Vec2>* outline;
	//vector<Box2d> colliders;
	//vector<Vec2> points;

	//
	rcHeightfield* solid;
	rcCompactHeightfield* chf;
	rcContourSet* cset;
	rcPolyMesh* pmesh;
	rcPolyMeshDetail* dmesh;
	vector<byte> triareas;
	//
	dtNavMeshQuery* nav_query;
	dtNavMesh* navmesh;
};
