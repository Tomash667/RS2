#include "EngineCore.h"
#include "GuiControls.h"
#include "Gui.h"
#include "Font.h"
#include "Input.h"


Panel::Layout Panel::default_layout;
Button::Layout Button::default_layout;
CheckBox::Layout CheckBox::default_layout;
ScrollBar::Layout ScrollBar::default_layout;
DropDownList::Layout DropDownList::default_layout;
ListBox::Layout ListBox::default_layout;
TextBox::Layout TextBox::default_layout;
DialogBox::Layout DialogBox::default_layout;


//-----------------------------------------------------------------------------
void Sprite::Draw()
{
	gui->DrawSprite(image, global_pos, size, color);
}


//-----------------------------------------------------------------------------
void Label::Draw()
{
	gui->DrawText(text, font, color, flags, Rect::Create(global_pos, size));
}

Int2 Label::CalculateSize() const
{
	Font* used_font = (font ? font : gui->GetDefaultFont());
	return used_font->CalculateSize(text);
}


//-----------------------------------------------------------------------------
void Panel::Draw()
{
	gui->DrawSpriteGrid(layout.image, layout.color, layout.corners.y, layout.corners.x, global_pos, size);
	Container::Draw();
}


//-----------------------------------------------------------------------------
void ProgressBar::Draw()
{
	gui->DrawSprite(background, global_pos, size);
	if(progress > 0.f)
		gui->DrawSpritePart(image, global_pos, size, Vec2(progress, 1));
}


//-----------------------------------------------------------------------------
void Button::Draw()
{
	Color font_color = layout.font_color;
	Texture* img;
	switch(state)
	{
	default:
	case UP:
		img = layout.image;
		break;
	case HOVER:
		img = layout.image_hover;
		break;
	case DISABLED:
		img = layout.image_disabled;
		font_color = layout.font_color_disabled;
		break;
	}
	gui->DrawSpriteGrid(img, Color::White, layout.corners.y, layout.corners.x, global_pos, size);
	Rect rect = Rect::Create(global_pos, size);
	gui->DrawText(text.c_str(), layout.font, font_color, Font::Center | Font::VCenter, rect, &rect);
}

void Button::Update(float dt)
{
	if(state == DISABLED)
		return;
	if(mouse_focus && Rect::IsInside(global_pos, size, gui->GetCursorPos()))
	{
		state = HOVER;
		if(gui->GetInput()->PressedOnce(Key::LeftButton) && event)
		{
			gui->TakeFocus(this);
			event(id);
		}
	}
	else
		state = UP;
}

void Button::CalculateSize(const Int2& padding)
{
	Font* font = (layout.font ? layout.font : gui->GetDefaultFont());
	size = font->CalculateSize(text) + padding;
}

void Button::NormalizeSize(Button* buttons[], uint count, const Int2& padding)
{
	Button::Layout& layout = buttons[0]->layout;
	Font* font = (layout.font ? layout.font : buttons[0]->gui->GetDefaultFont());
	Int2 max_size(0, 0);

	for(uint i = 0; i < count; ++i)
	{
		Button* bt = buttons[i];
		bt->size = font->CalculateSize(bt->text);
		max_size = Int2::Max(max_size, bt->size);
	}

	max_size += padding * 2;

	for(uint i = 0; i < count; ++i)
		buttons[i]->size = max_size;
}


//-----------------------------------------------------------------------------
void CheckBox::Draw()
{
	gui->DrawSprite(hover ? layout.hover : layout.background, global_pos, size);
	if(checked)
		gui->DrawSprite(layout.checkbox, global_pos, size);
}

void CheckBox::Update(float dt)
{
	if(mouse_focus && Rect::IsInside(global_pos, size, gui->GetCursorPos()))
	{
		focus = true;
		if(gui->GetInput()->PressedOnce(Key::LeftButton))
		{
			gui->TakeFocus(this);
			checked = !checked;
		}
	}
	else
		focus = false;
}


//-----------------------------------------------------------------------------
void ScrollBar::Draw()
{
	gui->DrawSpriteGrid(layout.background, Color::White, layout.corners.y, layout.corners.x, global_pos, size);

	Vec2 arrow_mid = Vec2(layout.arrow.image_region) / 2;
	if(horizontal)
	{
		// arrow left
		Matrix mat = Matrix::Transform2D(nullptr, 0.f, nullptr, &arrow_mid, PI, &Vec2(global_pos));
		gui->DrawSpriteComplex(layout.arrow.image[hover == HOVER_ARROW_LESS ? 1 : 0], layout.arrow.color, layout.arrow.image_region, layout.arrow.ToUV(), mat);

		// arrow right
		mat = Matrix::Transform2D(nullptr, 0.f, nullptr, nullptr, 0.f, &Vec2(Int2(global_pos.x + size.x - layout.arrow.image_region.x, global_pos.y)));
		gui->DrawSpriteComplex(layout.arrow.image[hover == HOVER_ARROW_MORE ? 1 : 0], layout.arrow.color, layout.arrow.image_region, layout.arrow.ToUV(), mat);
	}
	else
	{
		Int2 move = (layout.arrow.image_region.YX() - layout.arrow.image_region) / 2;

		// arrow top
		Matrix mat = Matrix::Transform2D(nullptr, 0.f, nullptr, &arrow_mid, PI * 3 / 2, &Vec2(global_pos + move));
		gui->DrawSpriteComplex(layout.arrow.image[hover == HOVER_ARROW_LESS ? 1 : 0], layout.arrow.color, layout.arrow.image_region, layout.arrow.ToUV(), mat);

		// arrow bottom
		mat = Matrix::Transform2D(nullptr, 0.f, nullptr, &arrow_mid, PI / 2,
			&Vec2(Int2(global_pos.x, global_pos.y + size.y - layout.arrow.image_region.x) + move));
		gui->DrawSpriteComplex(layout.arrow.image[hover == HOVER_ARROW_MORE ? 1 : 0], layout.arrow.color, layout.arrow.image_region, layout.arrow.ToUV(), mat);
	}

	// scroll
	Int2 scroll_pos, scroll_size;
	GetScrollPosSize(scroll_pos, scroll_size);
	gui->DrawSprite(nullptr, scroll_pos, scroll_size, hover == HOVER_SCROLL ? layout.scroll_hover_color : layout.scroll_color);
}

void ScrollBar::Update(float dt)
{
	Input* input = gui->GetInput();
	const Int2& cursor_pos = gui->GetCursorPos();
	if(!mouse_focus)
	{
		hover = HOVER_NONE;
		clicked = false;
	}
	else if(clicked)
	{
		if(input->Up(Key::LeftButton))
			clicked = false;
		else
		{
			Int2 scroll_pos, scroll_size;
			GetScrollPosSize(scroll_pos, scroll_size);
			Int2 dif = (cursor_pos - scroll_pos) - click_pt;
			if(horizontal)
			{
				scroll_pos.x = Clamp(scroll_pos.x + dif.x, global_pos.x + layout.arrow.image_region.x + layout.pad.x,
					global_pos.x + size.x - layout.arrow.image_region.x - layout.pad.x);
				int scroll_offset = scroll_pos.x - global_pos.x - layout.arrow.image_region.x - layout.pad.x;
				int scroll_offset_max = size.x - (layout.arrow.image_region.x + layout.pad.x) * 2 - scroll_size.x;
				scroll_offset = Clamp(scroll_offset, 0, scroll_offset_max);
				offset = float(scroll_offset) / scroll_offset_max * (total - part);
			}
			else
			{
				scroll_pos.y = Clamp(scroll_pos.y + dif.y, global_pos.y + layout.arrow.image_region.x + layout.pad.x,
					global_pos.y + size.y - layout.arrow.image_region.x - layout.pad.x);
				int scroll_offset = scroll_pos.y - global_pos.y - layout.arrow.image_region.x - layout.pad.x;
				int scroll_offset_max = size.y - (layout.arrow.image_region.x + layout.pad.x) * 2 - scroll_size.y;
				scroll_offset = Clamp(scroll_offset, 0, scroll_offset_max);
				offset = float(scroll_offset) / scroll_offset_max * (total - part);
			}
		}
	}
	else if(!Rect::IsInside(global_pos, size, cursor_pos))
		hover = HOVER_NONE;
	else
	{
		hover = HOVER_NONE;
		if(horizontal)
		{
			if(Rect::IsInside(global_pos, layout.arrow.image_region, cursor_pos))
				hover = HOVER_ARROW_LESS;
			else if(Rect::IsInside(Int2(global_pos.x + size.x - layout.arrow.image_region.x, global_pos.y), layout.arrow.image_region, cursor_pos))
				hover = HOVER_ARROW_MORE;
		}
		else
		{
			if(Rect::IsInside(global_pos, Int2(layout.arrow.image_region.y, layout.arrow.image_region.x), cursor_pos))
				hover = HOVER_ARROW_LESS;
			else if(Rect::IsInside(Int2(global_pos.x, global_pos.y + size.y - layout.arrow.image_region.x),
				Int2(layout.arrow.image_region.y, layout.arrow.image_region.x), cursor_pos))
				hover = HOVER_ARROW_MORE;
		}
		if(hover == HOVER_NONE)
		{
			Int2 scroll_pos, scroll_size;
			GetScrollPosSize(scroll_pos, scroll_size);
			if(Rect::IsInside(scroll_pos, scroll_size, cursor_pos))
			{
				hover = HOVER_SCROLL;
				if(input->Pressed(Key::LeftButton))
				{
					click_pt = cursor_pos - scroll_pos;
					clicked = true;
					gui->TakeFocus(this);
				}
			}
			else if(input->DownRepeat(Key::LeftButton))
			{
				bool less;
				if(horizontal)
					less = cursor_pos.x < scroll_pos.x;
				else
					less = cursor_pos.y < scroll_pos.y;
				if(less)
					offset = max(0.f, offset - part);
				else
					offset = min(float(total - part), offset + part);
			}
		}
		else if(input->DownRepeat(Key::LeftButton))
		{
			if(hover == HOVER_ARROW_LESS)
				offset = max(0.f, offset - click_step);
			else
				offset = min(float(total - part), offset + click_step);
		}

		int wheel = gui->GetInput()->GetMouseWheel();
		offset = Clamp(offset - wheel * click_step, 0.f, float(total - part));
	}
}

void ScrollBar::SetExtent(int part, int total)
{
	this->part = part;
	this->total = total;
}

void ScrollBar::GetScrollPosSize(Int2& scroll_pos, Int2& scroll_size)
{
	if(horizontal)
	{
		Int2 scroll_region = Int2(size.x - (layout.arrow.image_region.x + layout.pad.x) * 2, size.y - layout.pad.y * 2);
		if(part >= total || total == 0 || part == 0)
		{
			scroll_pos = Int2(global_pos.x + layout.arrow.image_region.x + layout.pad.x, global_pos.y + layout.pad.y);
			scroll_size = scroll_region;
		}
		else
		{
			scroll_size = Int2(float(part) / (total - part) * scroll_region.x, scroll_region.y);
			scroll_pos = Int2(global_pos.x + layout.arrow.image_region.x + layout.pad.x + int((offset / (total - part)) * (scroll_region.x - scroll_size.x)),
				global_pos.y + layout.pad.y);
		}
	}
	else
	{
		Int2 scroll_region = Int2(size.x - layout.pad.y * 2, size.y - (layout.arrow.image_region.x + layout.pad.x) * 2);
		if(part >= total || total == 0 || part == 0)
		{
			scroll_pos = Int2(global_pos.x + layout.pad.y, global_pos.y + layout.arrow.image_region.x + layout.pad.x);
			scroll_size = scroll_region;
		}
		else
		{
			scroll_size = Int2(scroll_region.x, float(part) / (total - part) * scroll_region.y);
			scroll_pos = Int2(global_pos.x + layout.pad.y,
				global_pos.y + layout.arrow.image_region.x + layout.pad.x + int((offset / (total - part)) * (scroll_region.y - scroll_size.y)));
		}
	}
}


//-----------------------------------------------------------------------------
void DropDownList::Init()
{
	size = Int2::Zero;
	Font* font = layout.font ? layout.font : gui->GetDefaultFont();
	for(Item& item : items)
	{
		Int2 item_size = font->CalculateSize(item.text);
		size = Int2::Max(size, item_size);
	}
	item_height = size.y + 2;
	total_height = items.size() * item_height;
	size += Int2(layout.pad * 2, layout.pad * 2 + layout.arrow.image_region.x);
}

void DropDownList::Draw()
{
	gui->DrawSpriteGrid(layout.background, Color::White, layout.corners.y, layout.corners.x, global_pos, size);
	gui->DrawSprite(layout.arrow.image[(hover || is_open) ? 1 : 0],
		Int2(global_pos.x + size.x - layout.arrow.image_region.x, global_pos.y + (size.y - layout.arrow.image_region.y) / 2),
		layout.arrow.image_size, layout.arrow.color);
	if(selected_index != -1)
	{
		Rect rect = Rect::Create(global_pos, size, 2);
		gui->DrawText(items[selected_index].text, layout.font, layout.font_color, Font::VCenter, rect, &rect);
	}
	if(is_open)
	{
		gui->DrawMenu([this]
		{
			gui->DrawSpriteGrid(layout.background, Color::White, layout.corners.y, layout.corners.x,
				Int2(global_pos.x, global_pos.y + size.y), Int2(size.x, total_height));
			int offset = global_pos.y + size.y + layout.pad;
			int index = 0;
			for(Item& item : items)
			{
				Rect rect = Rect(global_pos.x + layout.pad, offset, global_pos.x + size.x - layout.pad, offset + item_height);
				if(hover_index == index)
					gui->DrawSprite(nullptr, rect.p1, rect.Size(), layout.hover_color);
				gui->DrawText(item.text, layout.font, layout.font_color, Font::VCenter, rect);
				++index;
				offset += item_height;
			}
		});
	}
}

void DropDownList::Update(float dt)
{
	if(mouse_focus)
	{
		if(Rect::IsInside(global_pos, size, gui->GetCursorPos()))
		{
			hover = true;
			if(gui->GetInput()->PressedOnce(Key::LeftButton))
			{
				if(is_open)
					is_open = false;
				else
				{
					is_open = true;
					hover_index = -1;
				}
				gui->TakeFocus(this);
			}
		}
		else
			hover = false;

		if(is_open)
		{
			if(Rect::IsInside(global_pos + Int2(0, size.y), Int2(size.x, total_height), gui->GetCursorPos()))
			{
				hover_index = (gui->GetCursorPos().y - global_pos.y - size.y) / item_height;
				if(gui->GetInput()->PressedOnce(Key::LeftButton))
				{
					selected_index = hover_index;
					is_open = false;
				}
			}

			if(gui->GetInput()->PressedOnce(Key::Escape))
				is_open = false;
		}
	}
	else
	{
		hover = false;
	}

	if(is_open && !focus)
		is_open = false;
}


//-----------------------------------------------------------------------------
void ListBox::Init()
{
	Font* font = layout.font ? layout.font : gui->GetDefaultFont();
	item_height = font->height + layout.pad.y * 2;
}

void ListBox::Draw()
{
	gui->DrawSpriteGrid(layout.background, Color::White, layout.corners.y, layout.corners.x, global_pos, size);
	uint offset = 0;
	for(uint i = 0; i < items.size(); ++i)
	{
		Rect rect = Rect::Create(global_pos + Int2(layout.pad.x, offset), Int2(size.x - layout.pad.x * 2, item_height));
		if(i == selected_index)
			gui->DrawSprite(nullptr, Int2(global_pos.x, global_pos.y + offset), Int2(size.x, item_height), layout.selected_color);
		gui->DrawText(items[i].text, layout.font, layout.font_color, Font::VCenter, rect, &rect);
		offset += item_height;
	}
}

void ListBox::Update(float dt)
{
	if(mouse_focus && Rect::IsInside(global_pos, size, gui->GetCursorPos()) && gui->GetInput()->Pressed(Key::LeftButton))
	{
		gui->TakeFocus(this);
		int index = (gui->GetCursorPos().y - global_pos.y) / item_height;
		if(index < (int)items.size())
			selected_index = index;
	}
}


//-----------------------------------------------------------------------------
void TextBox::Draw()
{
	gui->DrawSpriteGrid(layout.background, layout.color, layout.corners.y, layout.corners.x, global_pos, size);
	Rect rect = Rect::Create(global_pos + layout.pad, size - layout.pad * 2);
	gui->DrawText(text, layout.font, layout.font_color, layout.flags, rect, &rect);
}


//-----------------------------------------------------------------------------
void DialogBox::Draw()
{
	gui->DrawSpriteGrid(layout.background, layout.background_color, layout.corners.y, layout.corners.x, global_pos, size);
	gui->DrawText(text, layout.font, layout.font_color, Font::Left, rect);
	button.Draw();
}

void DialogBox::Update(float dt)
{
	button.mouse_focus = mouse_focus;
	button.Update(dt);

	Input* input = gui->GetInput();
	if(input->PressedOnce(Key::Escape) || input->PressedOnce(Key::Enter) || input->PressedOnce(Key::Spacebar))
		gui->CloseDialog();
}

void DialogBox::OnEvent(int)
{
	gui->CloseDialog();
}
