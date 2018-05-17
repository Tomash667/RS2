#include "GameCore.h"
#include "Item.h"
#include <ResourceManager.h>

Item items[] = {
	{ "baseball_bat", "Baseball bat", nullptr },
	{ "medkit", "Medkit", nullptr }
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
		item.icon = res_mgr->GetTexture(Format("%s.png", item.id.c_str()));
}
