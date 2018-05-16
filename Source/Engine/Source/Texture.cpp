#include "EngineCore.h"
#include "Texture.h"
#include <d3d11_1.h>
#include "InternalHelper.h"

Texture::~Texture()
{
	SafeRelease(tex);
}
