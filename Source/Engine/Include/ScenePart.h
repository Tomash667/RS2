#pragma once

#include "QuadTree.h"

struct ScenePart : QuadTree::Part
{
	friend class Scene;

	~ScenePart();
	void Add(SceneNode* node);
	void Reset();

private:
	vector<SceneNode*> nodes;
};
