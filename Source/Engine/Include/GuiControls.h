#pragma once

#include "Control.h"

//-----------------------------------------------------------------------------
struct Sprite : Control
{
	Sprite() : image(nullptr) {}
	void Draw() override;

	Texture* image;
};

//-----------------------------------------------------------------------------
struct Label : Control
{
	Label() : font(nullptr), color(Color::Black) {}
	void Draw() override;
	Int2 CalculateSize() const;

	Font* font;
	string text;
	Color color;
};

//-----------------------------------------------------------------------------
struct Panel : Container
{
	Panel() : image(nullptr), corners(0,0), color(Color::White) {}
	void Draw() override;

	Texture* image;
	Int2 corners;
	Color color;
};

//-----------------------------------------------------------------------------
struct ProgressBar : Control
{
	ProgressBar() : image(nullptr), background(nullptr), progress(1.f) {}
	void Draw() override;

	Texture* image, *background;
	float progress;
};
