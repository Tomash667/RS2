#pragma once

#include "Vertex.h"
#include "Control.h"

class Gui : public Container
{
public:
	Gui();
	~Gui();
	void Init(Render* render, ResourceManager* res_mgr, Input* input);
	void Draw(const Matrix& mat_view_proj);
	void Update(float dt) override;
	bool To2dPoint(const Vec3& pos, Int2& pt);
	void ShowMessageBox(Cstring text);
	void ShowDialog(Control* control);
	void CloseDialog();
	void TakeFocus(Control* control);

	void DrawSprite(Texture* image, const Int2& pos, const Int2& size, Color color = Color::White);
	void DrawSpritePart(Texture* image, const Int2& pos, const Int2& size, const Vec2& part, Color color = Color::White);
	void DrawSpriteGrid(Texture* image, Color color, const GridF& pos, const GridF& uv);
	void DrawSpriteGrid(Texture* image, Color color, int image_size, int corner_size, const Int2& pos, const Int2& size);
	void DrawSpriteComplex(Texture* image, Color color, const Int2& size, const Box2d& uv, const Matrix& mat);
	bool DrawText(Cstring text, Font* font, Color color, int flags, const Rect& rect, const Rect* clip = nullptr);
	bool DrawTextOutline(Cstring text, Font* font, Color color, Color outline_color, int flags, const Rect& rect, const Rect* clip = nullptr);
	void DrawMenu(delegate<void()> draw_clbk) { this->draw_clbk = draw_clbk; }

	void SetCursorTexture(Texture* tex) { tex_cursor = tex; }
	void SetCursorVisible(bool visible) { cursor_visible = visible; }
	void SetWindowSize(const Int2& wnd_size);

	bool HaveDialog() { return dialog != nullptr; }
	bool IsCursorVisible() { return cursor_visible; }
	const Int2& GetCursorPos() { return cursor_pos; }
	Font* GetDefaultFont() { return default_font; }
	Input* GetInput() { return input; }
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

	Input* input;
	std::unique_ptr<GuiShader> gui_shader;
	Font* default_font;
	GuiVertex* v;
	uint in_buffer;
	vector<TextLine> lines;
	Int2 wnd_size, cursor_pos;
	Matrix mat_view_proj;
	Texture* tex_cursor;
	delegate<void()> draw_clbk;
	Control* focused, *dialog;
	Color dialog_overlay;
	bool cursor_visible, own_dialog;
};
