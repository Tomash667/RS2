#include "GameCore.h"
#include "Item.h"
#include <ResourceManager.h>
#include <Mesh.h>

Item items[] = {
	Item(Item::MELEE_WEAPON, "baseball_bat", "Baseball bat", "baseball_bat.qmsh", "baseball_bat.png", 45, 55),
	Item(Item::MELEE_WEAPON, "axe", "Axe", "axe.qmsh", "axe.png", 55, 104),
	Item(Item::RANGED_WEAPON, "pistol", "Pistol", "pistol.qmsh", "pistol.png", 81, 100),
	Item(Item::AMMO, "pistol_ammo", "Pistol ammo", "pistol_ammo.qmsh", "pistol_ammo.png"),
	Item(Item::MEDKIT, "medkit", "Medkit", "medkit.qmsh", "medkit_ico.png"),
	Item(Item::FOOD, "canned_food", "Canned food", "canned_food.qmsh", "canned_food.png")
};

Item* Item::Get(cstring id)
{
	if(id[0] == 0)
		return nullptr;
	for(Item& item : items)
	{
		if(item.id == id)
			return &item;
	}
	throw Format("Missing item '%s'.", id);
}

void Item::LoadData(ResourceManager* res_mgr)
{
	for(Item& item : items)
	{
		if(item.mesh_id)
		{
			item.mesh = res_mgr->GetMesh(item.mesh_id);
			Mesh::Point* point = item.mesh->FindPoint("ground");
			if(point)
			{
				item.ground_offset = Vec3::TransformZero(point->mat);
				item.ground_rot = point->rot;
				item.ground_rot.y = -item.ground_rot.y;
			}
		}
		if(item.icon_id)
			item.icon = res_mgr->GetTexture(item.icon_id);
	}
}
