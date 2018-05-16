#include "EngineCore.h"
#include "GuiControls.h"
#include "Gui.h"
#include "Font.h"


void Sprite::Draw()
{
	gui->DrawSprite(image, pos, size);
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
