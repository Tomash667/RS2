#include "EngineCore.h"
#include "GuiControls.h"
#include "Gui.h"
#include "Font.h"
#include "Input.h"


Button::Layout Button::default_layout;
DialogBox::Layout DialogBox::default_layout;
CheckBox::Layout CheckBox::default_layout;
DropDownList::Layout DropDownList::default_layout;


//-----------------------------------------------------------------------------
void Sprite::Draw()
{
	gui->DrawSprite(image, pos, size, color);
}


//-----------------------------------------------------------------------------
void Label::Draw()
{
	gui->DrawText(text, font, color, flags, Rect::Create(pos, size));
}

Int2 Label::CalculateSize() const
{
	Font* used_font = (font ? font : gui->GetDefaultFont());
	return used_font->CalculateSize(text);
}


//-----------------------------------------------------------------------------
void Panel::Draw()
{
	gui->DrawSpriteGrid(image, color, corners.y, corners.x, pos, size);
	Container::Draw();
}


//-----------------------------------------------------------------------------
void ProgressBar::Draw()
{
	gui->DrawSprite(background, pos, size);
	if(progress > 0.f)
		gui->DrawSpritePart(image, pos, size, Vec2(progress, 1));
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
	gui->DrawSpriteGrid(img, Color::White, layout.corners.y, layout.corners.x, pos, size);
	Rect rect = Rect::Create(pos, size);
	gui->DrawText(text.c_str(), layout.font, font_color, Font::Center | Font::VCenter, rect, &rect);
}

void Button::Update(float dt)
{
	if(state == DISABLED)
		return;
	if(mouse_focus && Rect::IsInside(pos, size, gui->GetCursorPos()))
	{
		state = HOVER;
		if(gui->GetInput()->PressedOnce(Key::LeftButton) && event)
			event(id);
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
	gui->DrawSprite(hover ? layout.hover : layout.background, pos, size);
	if(checked)
		gui->DrawSprite(layout.checkbox, pos, size);
}

void CheckBox::Update(float dt)
{
	if(mouse_focus && Rect::IsInside(pos, size, gui->GetCursorPos()))
	{
		focus = true;
		if(gui->GetInput()->PressedOnce(Key::LeftButton))
			checked = !checked;
	}
	else
		focus = false;
}


//-----------------------------------------------------------------------------
void ScrollBar::Draw()
{
	gui->DrawSpriteGrid(layout.background, Color::White, layout.corners.y, layout.corners.x, pos, size);
	if(horizontal)
	{
		Matrix::Transform2D(nullptr, 0.f, nullptr, &(Vec2(layout.arrow_size) / 2), PI, &Vec2::Zero);
		//gui->DrawSprite(hover == 1 ? layout.arrow_hover : layout.arrow,
	}
	else
	{

	}
}

void ScrollBar::Update(float dt)
{

}


//-----------------------------------------------------------------------------
void DropDownList::Draw()
{
	gui->DrawSpriteGrid(layout.background, Color::White, layout.corners.y, layout.corners.x, pos, size);
	if(selected_index != -1)
	{
		Rect rect = Rect::Create(pos, size, 2);
		gui->DrawText(items[selected_index].text, layout.font, layout.font_color, Font::Center | Font::VCenter, rect, &rect);
	}
	if(is_open)
	{
		// list
	}
}

void DropDownList::Update(float dt)
{
	if(mouse_focus)
	{
		if(Rect::IsInside(pos, size, gui->GetCursorPos()))
		{
			hover = true;
			if(gui->GetInput()->PressedOnce(Key::LeftButton))
			{
				if(is_open)
					is_open = false;
				else
				{
					is_open = true;
					hover_index = selected_index;
				}
				gui->TakeFocus(this);
			}
		}
		else
			hover = false;
	}
	else
	{
		hover = false;
	}

	if(is_open && !focus)
		is_open = false;
}


//-----------------------------------------------------------------------------
void DialogBox::Draw()
{
	gui->DrawSpriteGrid(layout.background, layout.background_color, layout.corners.y, layout.corners.x, pos, size);
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
