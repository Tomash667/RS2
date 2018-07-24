#include "GameCore.h"
#include "ObjectType.h"
#include <ResourceManager.h>

static ObjectType objects[] = {
	ObjectType("table", "table.qmsh"),
	ObjectType("bed", nullptr),
	ObjectType("lamp", nullptr),
	ObjectType("window", nullptr),
	ObjectType("shelves", nullptr),
	ObjectType("posters", nullptr),
	ObjectType("counter", nullptr)
};

void ObjectType::LoadData(ResourceManager* res_mgr)
{
	for(ObjectType& obj : objects)
	{
		if(obj.mesh_id)
			obj.mesh = res_mgr->GetMesh(obj.mesh_id);
	}
}
