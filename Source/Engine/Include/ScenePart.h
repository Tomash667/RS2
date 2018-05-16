#pragma once

#include "QuadTree.h"

struct ScenePart : QuadTree::Part
{
	friend class Scene;

	~ScenePart();
	void Add(SceneNode* node);
	
private:
	vector<SceneNode*> nodes;
};
