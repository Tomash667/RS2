#pragma once

#include "Resource.h"

struct Texture final : public Resource
{
	Texture(cstring name, ID3D11ShaderResourceView* tex) : Resource(name, Resource::Type::Texture), tex(tex) {}
	~Texture();

	ID3D11ShaderResourceView* tex;
};
