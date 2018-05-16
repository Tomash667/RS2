#pragma once

struct Item
{
	enum class Type
	{
		MeleeWeapon,
		RangedWeapon,
		Ammo,
		Food,
		Medicine
	};

	string id, name;
	Type type;
	int value, value2;
	Mesh* mesh;
	Texture* icon;

	static void LoadItems(ResourceManager* res_mgr);
};
