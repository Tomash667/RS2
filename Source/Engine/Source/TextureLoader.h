#pragma once

class TextureLoader
{
public:
	TextureLoader(ID3D11Device* device, ID3D11DeviceContext* device_context);
	Texture* Load(cstring name, cstring path);

private:
	ID3D11Device* device;
	ID3D11DeviceContext* device_context;
};
