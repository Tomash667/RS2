#include "GameCore.h"
#include "Item.h"
#include <ResourceManager.h>

Item items[] = {
	Item(Item::MELEE_WEAPON, "baseball_bat", "Baseball bat", nullptr, "baseball_bat.png"),
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
