#include "GameCore.h"
#include "Item.h"
#include <ResourceManager.h>

Item items[] = {
	Item(Item::MELEE_WEAPON, "baseball_bat", "Baseball bat", nullptr, "baseball_bat.png", 45, 55),
	//Item(Item::MELEE_WEAPON, "axe", "Axe", nullptr, "axe.png", 55, 104),
	Item(Item::RANGED_WEAPON, "pistol", "Pistol", "pistol.qmsh", "pistol.png", 81, 100),
	Item(Item::AMMO, "pistol_ammo", "Pistol ammo", "ammo.qmsh", "pistol_ammo.png"),
	Item(Item::MEDKIT, "medkit", "Medkit", "medkit.qmsh", "medkit_ico.png"),
	Item(Item::FOOD, "canned_food", "Canned food", "canned_food.qmsh", "canned_food.png")
};

Item* Item::Get(cstring id)
{
	for(Item& item : items)
	{
		if(item.id == id)
			return &item;
	}
	assert(0);
	return nullptr;
}

void Item::LoadData(ResourceManager* res_mgr)
{
	for(Item& item : items)
	{
		if(item.mesh_id)
			item.mesh = res_mgr->GetMesh(item.mesh_id);
		if(item.icon_id)
			item.icon = res_mgr->GetTexture(item.icon_id);
	}
}
