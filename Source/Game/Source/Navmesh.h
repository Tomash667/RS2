#pragma once

struct Navmesh
{
	struct Region
	{
		Vec3 pos[4];
	};

	~Navmesh() { DeleteElements(regions); }
	void Reset() { DeleteElements(regions); }

	vector<Region*> regions;
};
