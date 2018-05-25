#include "EngineCore.h"
#include "GuiControls.h"
#include "Gui.h"
#include "Font.h"
#include "Input.h"


Button::Layout Button::default_layout;


void Sprite::Draw()
{
	gui->DrawSprite(image, pos, size, color);
}


void Label::Draw()
{
	gui->DrawText(text, font, color, Font::Left, Rect::Create(pos, size));
}

Int2 Label::CalculateSize() const
{
	Font* used_font = (font ? font : gui->GetDefaultFont());
	return used_font->CalculateSize(text);
}


void Panel::Draw()
{
	gui->DrawSpriteGrid(image, color, corners.y, corners.x, pos, size);
	Container::Draw();
}


void ProgressBar::Draw()
{
	gui->DrawSprite(background, pos, size);
	if(progress > 0.f)
		gui->DrawSpritePart(image, pos, size, Vec2(progress, 1));
}


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
	case DOWN:
		img = layout.image_down;
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
	if(Rect::IsInside(pos, size, gui->GetCursorPos()))
	{
		if(gui->GetInput()->Down(Key::LeftButton))
			state = DOWN;
		else
		{
			if(state == DOWN && event)
				event(id);
			state = HOVER;
		}
	}
	else
		state = UP;
}
