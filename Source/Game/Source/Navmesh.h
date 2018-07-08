#pragma once

class Navmesh
{
public:
	void Reset();
	void StartRegion(const vector<Vec2>& outline) { this->outline = &outline; }
	void EndRegion();
	void AddCollier(const Box2d& box) { colliders.push_back(box); }
	void Draw(DebugDrawer* debug_drawer);

	// FIXME
	float dt;

private:
	void Triangulate(vector<p2t::Point*>* polyline, vector<p2t::Point*>* hole);

	struct Triangle
	{
		Vec2 pos[3];
	};
	vector<Triangle> triangles;

	// current batch, don't save
	const vector<Vec2>* outline;
	vector<Box2d> colliders;

	vector<vector<Vec2>> outlines;
};
