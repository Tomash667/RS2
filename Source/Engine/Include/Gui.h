#pragma once

#include "Vertex.h"
#include "Control.h"

class Gui : public Container
{
public:
	Gui();
	~Gui();
	void Init(Render* render, ResourceManager* res_mgr);
	void Draw(const Matrix& mat_view_proj);
	bool To2dPoint(const Vec3& pos, Int2& pt);

	void DrawSprite(Texture* image, const Int2& pos, const Int2& size, Color color = Color::White);
	void DrawSpritePart(Texture* image, const Int2& pos, const Int2& size, const Vec2& part, Color color = Color::White);
	void DrawSpriteGrid(Texture* image, Color color, const GridF& pos, const GridF& uv);
	void DrawSpriteGrid(Texture* image, Color color, int image_size, int corner_size, const Int2& pos, const Int2& size);
	bool DrawText(Cstring text, Font* font, Color color, int flags, const Rect& rect, const Rect* clip = nullptr);
	
	void SetWindowSize(const Int2& wnd_size);

	Font* GetDefaultFont() { return default_font; }
	const Int2& GetWindowSize() { return wnd_size; }

private:
	enum ClipResult
	{
		ClipNone,
		ClipAbove,
		ClipBelow,
		ClipRight,
		ClipLeft,
		ClipPartial
	};

	struct TextLine
	{
		uint begin, end;
		int width;

		TextLine(uint begin, uint end, int width) : begin(begin), end(end), width(width) {}
	};

	void Lock();
	void Flush(Texture* tex);
	void FillQuad(const Box2d& pos, const Box2d& tex, const Vec4& color);
	void SplitTextLines(cstring text, Font* font, int width, int flags);
	ClipResult Clip(int x, int y, int w, int h, const Rect* clip);
	void DrawTextLine(Font* font, cstring text, uint line_begin, uint line_end, const Vec4& color, int x, int y, const Rect* clip);

	std::unique_ptr<GuiShader> gui_shader;
	Font* default_font;
	GuiVertex* v;
	uint in_buffer;
	vector<TextLine> lines;
	Int2 wnd_size;
	Matrix mat_view_proj;
};
