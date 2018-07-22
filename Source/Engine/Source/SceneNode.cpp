#include "EngineCore.h"
#include "SceneNode.h"
#include "MeshInstance.h"

static int use_parent_bones_placeholder;
MeshPoint* SceneNode::USE_PARENT_BONES = (MeshPoint*)&use_parent_bones_placeholder;

SceneNode::~SceneNode()
{
	DeleteElements(childs);
	delete mesh_inst;
	delete container;
}

void SceneNode::Add(SceneNode* node, MeshPoint* point)
{
	assert(node && !node->parent);
	if(point)
	{
		if(point == USE_PARENT_BONES)
			assert(!node->mesh_inst && mesh_inst && node->mesh->IsUsingParentBones());
		else
			assert(mesh_inst && mesh->HavePoint((Mesh::Point*)point));
	}
	node->parent = this;
	node->parent_point = point;
	childs.push_back(node);
}

void SceneNode::SetParentPoint(MeshPoint* point)
{
	assert(point);
	Mesh::Point* pt = (Mesh::Point*)point;
	assert(parent && parent->mesh_inst && parent->mesh->HavePoint(pt));
	parent_point = pt;
}

MeshInstance* SceneNode::GetMeshInstance()
{
	if(mesh_inst)
		return mesh_inst;
	else if(parent_point == USE_PARENT_BONES)
		return parent->mesh_inst;
	else
		return nullptr;
}
