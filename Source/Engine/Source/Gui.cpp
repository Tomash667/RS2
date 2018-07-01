#include "EngineCore.h"
#include "Gui.h"
#include "GuiShader.h"
#include "GuiControls.h"
#include "ResourceManager.h"
#include "Font.h"
#include "Input.h"

Gui::Gui() : v(nullptr), cursor_visible(false), tex_cursor(nullptr), dialog(nullptr), dialog_overlay(Color(50, 50, 50, 150))
{
}

Gui::~Gui()
{
	if(own_dialog)
		delete dialog;
}

void Gui::Init(Render* render, ResourceManager* res_mgr, Input* input)
{
	assert(render && res_mgr && input);
	this->input = input;

	gui_shader.reset(new GuiShader(render));
	gui_shader->SetWindowSize(wnd_size);
	gui_shader->Init();

	default_font = res_mgr->GetFont("Arial", 14);

	Control::gui = this;
}

void Gui::Draw(const Matrix& mat_view_proj)
{
	this->mat_view_proj = mat_view_proj;

	gui_shader->Prepare();
	draw_clbk = nullptr;

	Container::Draw();

	if(HaveDialog())
	{
		if(dialog_overlay.a > 0)
			DrawSprite(nullptr, Int2::Zero, wnd_size, dialog_overlay);
		dialog->Draw();
	}

	if(draw_clbk)
		draw_clbk();

	if((cursor_visible || HaveDialog()) && tex_cursor)
		DrawSprite(tex_cursor, cursor_pos, Int2(32, 32));
}

void Gui::Update(float dt)
{
	if(cursor_visible || HaveDialog())
	{
		cursor_pos += input->GetMouseDif();
		cursor_pos.x = Clamp(cursor_pos.x, 0, wnd_size.x - 1);
		cursor_pos.y = Clamp(cursor_pos.y, 0, wnd_size.y - 1);
	}

	if(!HaveDialog())
	{
		mouse_focus = true;
		Container::Update(dt);
	}
	else
	{
		mouse_focus = false;
		Container::Update(dt);
		if(HaveDialog())
		{
			dialog->mouse_focus = true;
			dialog->Update(dt);
		}
	}

	if(focused && input->PressedOnce(Key::LeftButton))
	{
		focused->focus = false;
		focused = nullptr;
	}
}

void Gui::DrawSprite(Texture* image, const Int2& pos, const Int2& size, Color color)
{
	Lock();
	FillQuad(Box2d::Create(pos, size), Box2d::Unit, color);
	Flush(image ? image : gui_shader->GetEmptyTexture());
}

void Gui::DrawSpritePart(Texture* image, const Int2& pos, const Int2& size, const Vec2& part, Color color)
{
	Lock();
	FillQuad(Box2d::Create(pos, size * part), Box2d(Vec2::Zero, part), color);
	Flush(image);
}

void Gui::Lock()
{
	assert(!v);
	v = gui_shader->Lock();
	in_buffer = 0;
}

void Gui::Flush(Texture* tex)
{
	assert(tex && v);
	gui_shader->Draw(tex, in_buffer * 3);
	v = nullptr;
}

void Gui::FillQuad(const Box2d& pos, const Box2d& tex, const Vec4& color)
{
	v->pos = pos.LeftTop();
	v->tex = tex.LeftTop();
	v->color = color;
	++v;

	v->pos = pos.RightTop();
	v->tex = tex.RightTop();
	v->color = color;
	++v;

	v->pos = pos.LeftBottom();
	v->tex = tex.LeftBottom();
	v->color = color;
	++v;

	v->pos = pos.RightTop();
	v->tex = tex.RightTop();
	v->color = color;
	++v;

	v->pos = pos.RightBottom();
	v->tex = tex.RightBottom();
	v->color = color;
	++v;

	v->pos = pos.LeftBottom();
	v->tex = tex.LeftBottom();
	v->color = color;
	++v;

	in_buffer += 2;
}

bool Gui::DrawText(Cstring text, Font* font, Color color, int flags, const Rect& rect, const Rect* clip)
{
	if(!font)
		font = default_font;

	int width = rect.SizeX();
	Vec4 current_color = color;
	bool bottom_clip = false;

	Lock();
	SplitTextLines(text, font, width, flags);

	int y;
	if(IS_SET(flags, Font::Bottom))
		y = rect.Bottom() - int(lines.size())*font->height;
	else if(IS_SET(flags, Font::VCenter))
		y = rect.Top() + (rect.SizeY() - int(lines.size())*font->height) / 2;
	else
		y = rect.Top();

	for(TextLine& line : lines)
	{
		int x;
		if(IS_SET(flags, Font::Center))
			x = rect.Left() + (width - line.width) / 2;
		else if(IS_SET(flags, Font::Right))
			x = rect.Right() - line.width;
		else
			x = rect.Left();

		ClipResult clip_result = Clip(x, y, line.width, font->height, clip);
		if(clip_result == ClipNone)
			DrawTextLine(font, text, line.begin, line.end, current_color, x, y, nullptr);
		else if(clip_result == ClipPartial)
			DrawTextLine(font, text, line.begin, line.end, current_color, x, y, clip);
		else if(clip_result == ClipBelow)
		{
			// text is below visible region, stop drawing
			bottom_clip = true;
			break;
		}

		y += font->height;
	}

	Flush(font->tex);
	return !bottom_clip;
}

void Gui::SplitTextLines(cstring text, Font* font, int width, int flags)
{
	uint line_begin, line_end, line_index = 0;
	int line_width;
	uint text_end = (uint)strlen(text);

	lines.clear();
	while(font->SplitLine(line_begin, line_end, line_width, line_index, text, text_end, flags, width))
		lines.push_back(TextLine(line_begin, line_end, line_width));
}

void Gui::DrawTextLine(Font* font, cstring text, uint line_begin, uint line_end, const Vec4& color, int x, int y, const Rect* clip)
{
	for(uint i = line_begin; i < line_end; ++i)
	{
		Font::Glyph& glyph = font->glyph[byte(text[i])];
		Int2 glyph_size = Int2(glyph.width, font->height);

		ClipResult clip_result = Clip(x, y, glyph_size.x, glyph_size.y, clip);
		if(clip_result == ClipNone)
			FillQuad(Box2d::Create(Int2(x, y), glyph_size), glyph.uv, color);
		else if(clip_result == ClipPartial)
		{
			Box2d orig_pos = Box2d::Create(Int2(x, y), glyph_size);
			Box2d clip_pos(float(max(x, clip->Left())), float(max(y, clip->Top())),
				float(min(x + glyph_size.x, clip->Right())), float(min(y + glyph_size.y, clip->Bottom())));
			Vec2 orig_size = orig_pos.Size();
			Vec2 clip_size = clip_pos.Size();
			Vec2 s(clip_size.x / orig_size.x, clip_size.y / orig_size.y);
			Vec2 shift = clip_pos.v1 - orig_pos.v1;
			shift.x /= orig_size.x;
			shift.y /= orig_size.y;
			Vec2 uv_size = glyph.uv.Size();
			Box2d clip_uv(glyph.uv.v1 + Vec2(shift.x*uv_size.x, shift.y*uv_size.y));
			clip_uv.v2 += Vec2(uv_size.x*s.x, uv_size.y*s.y);
			FillQuad(clip_pos, clip_uv, color);
		}
		else if(clip_result == ClipRight)
		{
			// text is outside visible region on right, stop drawing line
			break;
		}

		x += glyph_size.x;
		if(in_buffer * 3 == GuiShader::MaxVertex)
		{
			Flush(font->tex);
			Lock();
		}
	}
}

bool Gui::DrawTextOutline(Cstring text, Font* font, Color color, Color outline_color, int flags, const Rect& rect, const Rect* clip)
{
	Rect outline_rect = rect + Int2(3, 3);
	DrawText(text, font, outline_color, flags, outline_rect, clip);
	return DrawText(text, font, color, flags, rect, clip);
}

Gui::ClipResult Gui::Clip(int x, int y, int w, int h, const Rect* clip)
{
	if(!clip)
		return ClipNone;
	else if(x >= clip->Left() && y >= clip->Top() && x + w < clip->Right() && y + h < clip->Bottom())
		return ClipNone;
	else if(y + h < clip->Top())
		return ClipAbove;
	else if(y > clip->Bottom())
		return ClipBelow;
	else if(x > clip->Right())
		return ClipRight;
	else if(x + w < clip->Left())
		return ClipLeft;
	else
		return ClipPartial;
}

void Gui::DrawSpriteGrid(Texture* image, Color color, const GridF& pos, const GridF& uv)
{
	assert(pos.size == uv.size);
	Lock();
	Vec4 current_color = color;
	for(uint y = 0; y < pos.size - 1; ++y)
	{
		for(uint x = 0; x < pos.size - 1; ++x)
		{
			v->pos = pos(x, y);
			v->tex = uv(x, y);
			v->color = current_color;
			++v;

			v->pos = pos(x + 1, y);
			v->tex = uv(x + 1, y);
			v->color = current_color;
			++v;

			v->pos = pos(x, y + 1);
			v->tex = uv(x, y + 1);
			v->color = current_color;
			++v;

			v->pos = pos(x + 1, y);
			v->tex = uv(x + 1, y);
			v->color = current_color;
			++v;

			v->pos = pos(x + 1, y + 1);
			v->tex = uv(x + 1, y + 1);
			v->color = current_color;
			++v;

			v->pos = pos(x, y + 1);
			v->tex = uv(x, y + 1);
			v->color = current_color;
			++v;

			in_buffer += 2;
		}
	}
	Flush(image);
}

void Gui::DrawSpriteGrid(Texture* image, Color color, int image_size, int corner_size, const Int2& pos, const Int2& size)
{
	if(corner_size > 0)
	{
		assert(image_size > 0
			&& corner_size > 0
			&& image_size - corner_size * 2 > 0
			&& size.x >= corner_size * 2
			&& size.y >= corner_size * 2
			&& size.x >= (image_size - corner_size * 2)
			&& size.y >= (image_size - corner_size * 2));
		static GridF pos_grid(4), uv_grid(4);
		uv_grid.Set({ 0.f, float(corner_size) / image_size, float(image_size - corner_size) / image_size, 1.f });
		pos_grid.Set({ pos.x, pos.x + corner_size, pos.x + size.x - corner_size, pos.x + size.x },
			{ pos.y, pos.y + corner_size, pos.y + size.y - corner_size, pos.y + size.y });
		DrawSpriteGrid(image, color, pos_grid, uv_grid);
	}
	else
		DrawSprite(image, pos, size, color);
}

void Gui::DrawSpriteComplex(Texture* image, Color color, const Int2& size, const Box2d& tex, const Matrix& mat)
{
	assert(image);
	Lock();
	Vec4 current_color = color;
	
	v->pos = Vec2::Transform(Vec2::Zero, mat);
	v->tex = tex.LeftTop();
	v->color = color;
	++v;

	v->pos = Vec2::Transform(Vec2((float)size.x, 0), mat);
	v->tex = tex.RightTop();
	v->color = color;
	++v;

	v->pos = Vec2::Transform(Vec2(0, (float)size.y), mat);
	v->tex = tex.LeftBottom();
	v->color = color;
	++v;

	v->pos = Vec2::Transform(Vec2(0, (float)size.y), mat);
	v->tex = tex.LeftBottom();
	v->color = color;
	++v;

	v->pos = Vec2::Transform(Vec2((float)size.x, 0), mat);
	v->tex = tex.RightTop();
	v->color = color;
	++v;

	v->pos = Vec2::Transform(Vec2(size), mat);
	v->tex = tex.RightBottom();
	v->color = color;
	++v;

	in_buffer += 2;
	Flush(image);

}

bool Gui::To2dPoint(const Vec3& pos, Int2& pt)
{
	Vec4 v4;
	Vec3::Transform(pos, mat_view_proj, v4);

	if(v4.z < 0)
	{
		// behind camera
		return false;
	}

	// see if we are in world space already
	Vec3 v3(v4.x, v4.y, v4.z);
	if(v4.w != 1)
	{
		if(v4.w == 0)
			v4.w = 0.00001f;
		v3 /= v4.w;
	}

	pt.x = int(v3.x*(wnd_size.x / 2) + (wnd_size.x / 2));
	pt.y = -int(v3.y*(wnd_size.y / 2) - (wnd_size.y / 2));

	return true;
}

void Gui::SetWindowSize(const Int2& wnd_size)
{
	float aspect = float(wnd_size.x) / wnd_size.y;
	Int2 new_wnd_size = Int2(int(aspect * 800), 800);
	if(this->wnd_size != new_wnd_size)
	{
		this->wnd_size = new_wnd_size;
		if(gui_shader)
			gui_shader->SetWindowSize(this->wnd_size);
		cursor_pos = this->wnd_size / 2;
		Container::Event(G_CHANGED_RESOLUTION);
		if(dialog)
			dialog->SetPos((this->wnd_size - dialog->size) / 2);
	}
}

void Gui::ShowMessageBox(Cstring text)
{
	if(HaveDialog())
		CloseDialog();

	DialogBox* dialog = new DialogBox;
	dialog->text = text;
	dialog->button.text = "OK";
	dialog->button.CalculateSize();
	Font* font = dialog->layout.font ? dialog->layout.font : GetDefaultFont();
	Int2 text_size = font->CalculateSize(dialog->text, wnd_size.x - 100);
	dialog->size.x = max(text_size.x, dialog->button.size.x) + 10 + dialog->layout.corners.x * 2;
	dialog->size.y = text_size.y + dialog->button.size.y + 15 + dialog->layout.corners.x * 2;
	Int2 pos = (wnd_size - dialog->size) / 2;
	dialog->SetPos(pos);
	dialog->button.SetPos(Int2((wnd_size.x - dialog->button.size.x) / 2, pos.y + 10 + dialog->layout.corners.x + text_size.y));
	dialog->rect = Rect(pos.x + dialog->layout.corners.x + 5, pos.y + dialog->layout.corners.x + 5);
	dialog->rect.p2 += text_size;
	dialog->button.event = delegate<void(int)>(dialog, &DialogBox::OnEvent);
	dialog->parent = this;

	this->dialog = dialog;
	own_dialog = true;
}

void Gui::ShowDialog(Control* control)
{
	if(HaveDialog())
		CloseDialog();

	control->SetPos(Int2((wnd_size.x - control->size.x) / 2, (wnd_size.y - control->size.y) / 2));
	control->visible = true;
	control->parent = this;
	dialog = control;
	own_dialog = false;
}

void Gui::CloseDialog()
{
	assert(HaveDialog());
	if(own_dialog)
		delete dialog;
	else
	{
		dialog->visible = false;
		dialog->parent = nullptr;
	}
	dialog = nullptr;
}

void Gui::TakeFocus(Control* control)
{
	assert(control);
	if(control == focused)
		return;
	if(focused)
		focused->focus = false;
	focused = control;
	focused->focus = true;
}
