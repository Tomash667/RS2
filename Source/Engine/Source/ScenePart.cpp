#include "EngineCore.h"
#include "ScenePart.h"
#include "SceneNode.h"

ScenePart::~ScenePart()
{
	DeleteElements(nodes);
}

void ScenePart::Add(SceneNode* node)
{
	assert(node);
	nodes.push_back(node);
}

void ScenePart::Reset()
{
	DeleteElements(nodes);
}
