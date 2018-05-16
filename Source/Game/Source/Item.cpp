#include "GameCore.h"
#include "Item.h"
#include <ResourceManager.h>

Item items[] = {
	"bat", "Baseball bat", Item::Type::MeleeWeapon, 45, 55, nullptr, nullptr,
	"axe", "Axe", Item::Type::MeleeWeapon, 55, 104, nullptr, nullptr,
	"pistol", "Pistol", Item::Type::RangedWeapon, 81, 100, nullptr, nullptr,
	"ammo", "Pistol ammo", Item::Type::Ammo, 0, 0, nullptr, nullptr,
	"food", "Food", Item::Type::Food, 0, 0, nullptr, nullptr,
	"medkit", "Medkit", Item::Type::Medicine, 0, 0, nullptr, nullptr
};

void Item::LoadItems(ResourceManager* res_mgr)
{
	for(Item& item : items)
	{
		item.mesh = res_mgr->GetMesh(Format("%s.qmsh", item.id.c_str()));
		item.icon = res_mgr->GetTexture(Format("%s.png", item.id.c_str()));
	}
}
