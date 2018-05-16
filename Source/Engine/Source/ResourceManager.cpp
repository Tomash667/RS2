#include "EngineCore.h"
#include "ResourceManager.h"
#include "Render.h"
#include "TextureLoader.h"
#include "QmshLoader.h"
#include "FontLoader.h"
#include "Texture.h"
#include "Mesh.h"
#include "Font.h"

ResourceManager::ResourceManager() : render(nullptr)
{
}

ResourceManager::~ResourceManager()
{
	DeleteElements(resources);
}

void ResourceManager::Init(Render* render)
{
	assert(render);
	this->render = render;

	ID3D11Device* device = render->GetDevice();
	ID3D11DeviceContext* device_context = render->GetDeviceContext();
	tex_loader.reset(new TextureLoader(device, device_context));
	qmsh_loader.reset(new QmshLoader(this, device, device_context));
	font_loader.reset(new FontLoader(device));
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

Mesh* ResourceManager::GetMesh(Cstring name)
{
	Mesh* mesh = (Mesh*)Get(name, Resource::Type::Mesh);
	if(!mesh)
	{
		cstring path = Format("Data/%s", name);
		mesh = qmsh_loader->Load(name, path);
		resources.insert(mesh);
	}
	return mesh;
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
