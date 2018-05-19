#pragma once

struct Item
{
	enum Type
	{
		MELEE_WEAPON,
		MEDKIT,
		FOOD
	};

	Item(Type type, cstring id, cstring name, cstring mesh_id, cstring icon_id) : type(type), id(id), name(name), mesh_id(mesh_id), icon_id(icon_id),
		mesh(nullptr), icon(nullptr) {}

	Type type;
	cstring id, name, mesh_id, icon_id;
	Mesh* mesh;
	Texture* icon;

	static Item* Get(cstring id);
	static void LoadData(ResourceManager* res_mgr);
};
