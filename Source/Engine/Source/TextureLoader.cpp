#include "EngineCore.h"
#include "TextureLoader.h"
#include "WICTextureLoader.h"
#include "Texture.h"

TextureLoader::TextureLoader(ID3D11Device* device, ID3D11DeviceContext* device_context) : device(device), device_context(device_context)
{
	assert(device && device_context);
}

Texture* TextureLoader::Load(cstring name, cstring path)
{
	ID3D11ShaderResourceView* view;
	const wchar_t* wpath = ToWString(path);
	HRESULT result = CreateWICTextureFromFile(device, device_context, wpath, nullptr, &view);
	if(FAILED(result))
		throw Format("Failed to load texture '%s' (%u).", path, result);
	return new Texture(name, view);
}
