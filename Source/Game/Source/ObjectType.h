#pragma once

struct ObjectType
{
	ObjectType(cstring id, cstring mesh_id) : id(id), mesh_id(mesh_id) {}

	cstring id, mesh_id;
	Mesh* mesh;

	static void LoadData(ResourceManager* res_mgr);
};
