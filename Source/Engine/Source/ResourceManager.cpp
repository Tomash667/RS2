#include "EngineCore.h"
#include "ResourceManager.h"
#include "Render.h"
#include "TextureLoader.h"
#include "QmshLoader.h"
#include "FontLoader.h"
#include "SoundLoader.h"
#include "Texture.h"
#include "Mesh.h"
#include "Font.h"
#include "Sound.h"

ResourceManager::ResourceManager()
{
}

ResourceManager::~ResourceManager()
{
	DeleteElements(resources);
}

void ResourceManager::Init(Render* render, SoundManager* sound_mgr)
{
	assert(render && sound_mgr);

	ID3D11Device* device = render->GetDevice();
	ID3D11DeviceContext* device_context = render->GetDeviceContext();
	tex_loader.reset(new TextureLoader(device, device_context));
	qmsh_loader.reset(new QmshLoader(this, device, device_context));
	font_loader.reset(new FontLoader(device));
	sound_loader.reset(new SoundLoader(sound_mgr));
}

Resource* ResourceManager::Get(cstring name, Resource::Type type)
{
	assert(name);

	static Resource res_search;
	res_search.name = name;
	auto it = resources.find(&res_search);
	if(it == resources.end())
		return nullptr;
	if((*it)->type != type)
		throw Format("Resource '%s' type mismatch.", name);
	return *it;
}

Font* ResourceManager::GetFont(Cstring name, int size)
{
	assert(size >= 1);
	cstring resource_name = Format("%s;%d", name, size);
	Font* font = (Font*)Get(resource_name, Resource::Type::Font);
	if(!font)
	{
		font = font_loader->Load(name, size);
		resources.insert(font);
	}
	return font;
}

Mesh* ResourceManager::GetMesh(Cstring name)
{
	Mesh* mesh = (Mesh*)Get(name, Resource::Type::Mesh);
	if(!mesh)
	{
		cstring path = Format("Data/%s", name);
		mesh = qmsh_loader->Load(name, path, false);
		resources.insert(mesh);
	}
	return mesh;
}

Mesh* ResourceManager::GetMeshRaw(Cstring name)
{
	Mesh* mesh = (Mesh*)Get(name, Resource::Type::Mesh);
	if(!mesh)
	{
		cstring path = Format("Data/%s", name);
		mesh = qmsh_loader->Load(name, path, true);
		resources.insert(mesh);
	}
	return mesh;
}

Music* ResourceManager::GetMusic(Cstring name)
{
	Music* music = (Music*)Get(name, Resource::Type::Music);
	if(!music)
	{
		cstring path = Format("Data/%s", name);
		music = sound_loader->LoadMusic(name, path);
		resources.insert(music);
	}
	return music;
}

Sound* ResourceManager::GetSound(Cstring name)
{
	Sound* sound = (Sound*)Get(name, Resource::Type::Sound);
	if(!sound)
	{
		cstring path = Format("Data/%s", name);
		sound = sound_loader->LoadSound(name, path);
		resources.insert(sound);
	}
	return sound;
}

Texture* ResourceManager::GetTexture(Cstring name)
{
	Texture* tex = (Texture*)Get(name, Resource::Type::Texture);
	if(!tex)
	{
		cstring path = Format("Data/%s", name);
		tex = tex_loader->Load(name, path);
		resources.insert(tex);
	}
	return tex;
}

void ResourceManager::AddFontFromFile(Cstring name)
{
	font_loader->AddFromFile(Format("Data/%s", name));
}

Mesh* ResourceManager::CreateMesh(MeshBuilder* mesh_builder)
{
	return qmsh_loader->Create(mesh_builder);
}
