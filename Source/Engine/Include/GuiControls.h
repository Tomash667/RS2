#pragma once

#include "Control.h"

//-----------------------------------------------------------------------------
struct Sprite : Control
{
	Sprite() : image(nullptr), color(Color::White) {}
	void Draw() override;

	Texture* image;
	Color color;
};

//-----------------------------------------------------------------------------
struct Label : Control
{
	Label() : font(nullptr), color(Color::Black), flags(0) {}
	void Draw() override;
	Int2 CalculateSize() const;

	Font* font;
	string text;
	Color color;
	int flags;
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

//-----------------------------------------------------------------------------
struct Button : Control
{
	enum State
	{
		UP,
		HOVER,
		DISABLED
	};

	struct Layout
	{
		Texture* image, *image_hover, *image_disabled;
		Int2 corners;
		Font* font;
		Color font_color, font_color_disabled;
	};

	Button() : layout(default_layout), id(0), event(nullptr), state(UP) {}
	Button(Layout& layout) : layout(layout), id(0), event(nullptr), state(UP) {}
	void Draw() override;
	void Update(float dt) override;

	Layout& layout;
	int id;
	delegate<void(int)> event;
	State state;
	string text;

	static void NormalizeSize(Button* buttons[], uint count, const Int2& padding);
	static Layout default_layout;
};
