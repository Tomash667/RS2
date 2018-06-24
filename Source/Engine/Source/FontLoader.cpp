#include "EngineCore.h"
#include "FontLoader.h"
#include "Font.h"
#include "Render.h"
#include "Texture.h"
#include <objidl.h>
#include <gdiplus.h>
#include <d3d11_1.h>
#include "InternalHelper.h"

FontLoader::FontLoader(ID3D11Device* device) : device(device), gdi_initialized(false)
{
}

Font* FontLoader::Load(cstring name, int size)
{
	InitGdi();

	const int weight = FW_NORMAL;

	Ptr<Font> font(new Font(Format("%s;%d", name, size)));
	HDC hdc = GetDC(nullptr);
	int logic_size = -MulDiv(size, 96, 72);
	//int logic_size = -MulDiv(size, GetDeviceCaps(hdc, LOGPIXELSX), 72);

	// create winapi font
	HFONT winapi_font = CreateFontA(logic_size, 0, 0, 0, weight, false, false, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, name);
	if(!winapi_font)
	{
		DWORD error = GetLastError();
		ReleaseDC(nullptr, hdc);
		throw Format("Failed to create font '%s' (%u).", name, error);
	}

	// get glyphs weights, font height
	int glyph_w[256];
	SelectObject(hdc, (HGDIOBJ)winapi_font);
	if(GetCharWidth32(hdc, 0, 255, glyph_w) == 0)
	{
		ABC abc[256];
		if(GetCharABCWidths(hdc, 0, 255, abc) == 0)
		{
			DWORD error = GetLastError();
			DeleteObject(winapi_font);
			ReleaseDC(nullptr, hdc);
			throw Format("Failed to get font '%s' glyphs (%u).", name, error);
		}
		for(int i = 0; i <= 255; ++i)
		{
			ABC& a = abc[i];
			glyph_w[i] = a.abcA + a.abcB + a.abcC;
		}
	}
	TEXTMETRIC tm;
	GetTextMetricsA(hdc, &tm);
	font->height = tm.tmHeight;
	ReleaseDC(nullptr, hdc);

	// calculate texture size
	Int2 tex_size(padding * 2, padding * 2 + font->height);
	for(int i = 32; i <= 255; ++i)
	{
		int width = glyph_w[i];
		if(width)
			tex_size.x += width + padding;
	}
	tex_size.x = NextPow2(tex_size.x);
	tex_size.y = NextPow2(tex_size.y);

	// setup glyphs
	Int2 offset(padding, padding);
	for(int i = 0; i < 32; ++i)
		font->glyph[i].width = 0;
	for(int i = 32; i <= 255; ++i)
	{
		Font::Glyph& glyph = font->glyph[i];
		glyph.width = glyph_w[i];
		if(glyph.width)
		{
			glyph.uv.v1 = Vec2(float(offset.x) / tex_size.x, float(offset.y) / tex_size.y);
			glyph.uv.v2 = glyph.uv.v1 + Vec2(float(glyph.width) / tex_size.x, float(font->height) / tex_size.y);
			offset.x += glyph.width + padding;
		}
	}

	// create texture
	D3D11_TEXTURE2D_DESC desc = { 0 };
	desc.Width = tex_size.x;
	desc.Height = tex_size.y;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = D3D11_RESOURCE_MISC_GDI_COMPATIBLE;

	CPtr<ID3D11Texture2D> tex;
	HRESULT result = device->CreateTexture2D(&desc, nullptr, tex);
	if(FAILED(result))
	{
		DeleteObject(winapi_font);
		throw Format("Failed to create font texture '%s' (%ux%u, result %u).", name, tex_size.x, tex_size.y, result);
	}

	// render font to texture
	RenderFontToTexture(tex, font, winapi_font);
	DeleteObject(winapi_font);

	// create texture view
	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Format = desc.Format;

	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Texture2D.MipLevels = 1;

	ID3D11ShaderResourceView* texture_view;
	C(device->CreateShaderResourceView(tex, &SRVDesc, &texture_view));

	font->tex = new Texture(Format("font %s (%u)", name, size), texture_view);

	// make tab size of 4 spaces
	Font::Glyph& tab = font->glyph['\t'];
	Font::Glyph& space = font->glyph[' '];
	tab.width = space.width * 4;;
	tab.uv = space.uv;

	return font.Pin();
}

void FontLoader::InitGdi()
{
	if(gdi_initialized)
		return;

	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	gdiplusStartupInput.GdiplusVersion = 1;
	gdiplusStartupInput.DebugEventCallback = nullptr;
	gdiplusStartupInput.SuppressBackgroundThread = TRUE;
	gdiplusStartupInput.SuppressExternalCodecs = TRUE;
	ULONG_PTR gdiplusToken = 0;
	Gdiplus::GdiplusStartupOutput output;

	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, &output);
	gdi_initialized = true;
}

void FontLoader::RenderFontToTexture(ID3D11Texture2D* tex, Font* font, void* winapi_font)
{
	IDXGISurface1* surface;
	C(tex->QueryInterface(__uuidof(IDXGISurface1), (void**)&surface));
	HDC hdc;
	C(surface->GetDC(TRUE, &hdc));

	Gdiplus::Graphics graphics(hdc);
	graphics.SetTextRenderingHint(Gdiplus::TextRenderingHint::TextRenderingHintAntiAlias);
	Gdiplus::SolidBrush brush(Gdiplus::Color(255, 255, 255, 255));
	Gdiplus::Font gdi_font(hdc, (HFONT)winapi_font);
	Gdiplus::PointF point;
	const Gdiplus::StringFormat* format = Gdiplus::StringFormat::GenericTypographic();

	Int2 offset(padding, padding);
	wchar_t wc[4];
	char c[2];
	c[1] = 0;
	for(int i = 32; i <= 255; ++i)
	{
		Font::Glyph& glyph = font->glyph[i];
		if(glyph.width == 0)
			continue;

		c[0] = (char)i;
		mbstowcs(wc, c, 4);
		point.X = (float)offset.x;
		point.Y = (float)offset.y;
		graphics.DrawString(wc, 1, &gdi_font, point, format, &brush);

		offset.x += glyph.width + padding;
	}

	C(surface->ReleaseDC(nullptr));
	surface->Release();
}

void FontLoader::AddFromFile(cstring name)
{
	int result = AddFontResourceEx(name, FR_PRIVATE, 0);
	if(result == 0)
		throw Format("Failed to add font '%s' (%u).", name, GetLastError());
}
