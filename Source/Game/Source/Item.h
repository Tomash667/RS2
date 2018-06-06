#pragma once

struct Item
{
	enum Type
	{
		MELEE_WEAPON,
		RANGED_WEAPON,
		AMMO,
		MEDKIT,
		FOOD
	};

	Item(Type type, cstring id, cstring name, cstring mesh_id, cstring icon_id, int value = 0, int value2 = 0) : type(type), id(id), name(name), mesh_id(mesh_id), icon_id(icon_id),
		mesh(nullptr), icon(nullptr), value(value), value2(value2), ground_offset(Vec3::Zero), ground_rot(Vec3::Zero) {}
	int RandomValue() const { return Random(value, value2); }

	Type type;
	cstring id, name, mesh_id, icon_id;
	Mesh* mesh;
	Texture* icon;
	int value, value2;
	Vec3 ground_offset, ground_rot;

	static Item* Get(Cstring id);
	static void LoadData(ResourceManager* res_mgr);
};
