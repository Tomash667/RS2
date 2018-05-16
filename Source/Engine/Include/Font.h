#pragma once

#include "Resource.h"

struct Font : Resource
{
	enum Flags
	{
		Left = 0,
		Center = 1,
		Right = 2,
		Top = 0,
		VCenter = 4,
		Bottom = 8,
		SingleLine = 16
	};

	struct Glyph
	{
		Box2d uv;
		int width;
	};

	Font(cstring name) : Resource(name, Resource::Type::Font), tex(nullptr) {}
	~Font();
	bool SplitLine(uint& out_begin, uint& out_end, int& out_width, uint& in_out_index, cstring text, uint text_end, int flags, int width) const;
	int GetCharWidth(char c) const { return glyph[(byte)c].width; }
	Int2 CalculateSize(Cstring text, int limit_width = std::numeric_limits<int>::max()) const;

	Texture* tex;
	Glyph glyph[256];
	int height;
};
