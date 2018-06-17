#pragma once

#include "Resource.h"

class ResourceManager
{
public:
	ResourceManager();
	~ResourceManager();
	void Init(Render* render, SoundManager* sound_mgr);

	void AddFontFromFile(Cstring name);
	Font* GetFont(Cstring name, int size);
	Mesh* GetMesh(Cstring name);
	Mesh* GetMeshRaw(Cstring name);
	Music* GetMusic(Cstring name);
	Sound* GetSound(Cstring name);
	Texture* GetTexture(Cstring name);
	Mesh* CreateMesh(MeshBuilder* mesh_builder);

private:
	struct ResourceComparer
	{
		bool operator () (const Resource* r1, const Resource* r2) const
		{
			return _stricmp(r1->name.c_str(), r2->name.c_str()) > 0;
		}
	};
	typedef std::set<Resource*, ResourceComparer> ResourceContainer;
	typedef ResourceContainer::iterator ResourceIterator;

	Resource* Get(cstring name, Resource::Type type);

	unique_ptr<TextureLoader> tex_loader;
	unique_ptr<QmshLoader> qmsh_loader;
	unique_ptr<FontLoader> font_loader;
	unique_ptr<SoundLoader> sound_loader;
	ResourceContainer resources;
	Resource res_search;
};
