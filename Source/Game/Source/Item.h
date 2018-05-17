#pragma once

struct Item
{
	cstring id, name, icon_id;
	Texture* icon;

	static Item* Get(cstring id);
	static void LoadData(ResourceManager* res_mgr);
};
