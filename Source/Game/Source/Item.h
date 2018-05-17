#pragma once

struct Item
{
	string id, name;
	Texture* icon;

	static Item* Get(cstring id);
	static void LoadData(ResourceManager* res_mgr);
};
