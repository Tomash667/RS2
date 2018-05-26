#pragma once

class FontLoader
{
public:
	FontLoader(ID3D11Device* device);
	Font* Load(cstring name, int size);
	void AddFromFile(cstring name);

private:
	void InitGdi();
	void RenderFontToTexture(ID3D11Texture2D* tex, Font* font, void* winapi_font);

	ID3D11Device* device;
	const int padding = 2;
	bool gdi_initialized;
};
