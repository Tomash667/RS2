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
	void CalculateSize(const Int2& padding = Int2(10, 10));

	Layout& layout;
	int id;
	delegate<void(int)> event;
	State state;
	string text;

	static void NormalizeSize(Button* buttons[], uint count, const Int2& padding);
	static Layout default_layout;
};

//-----------------------------------------------------------------------------
struct CheckBox : Control
{
	bool checked;
};

//-----------------------------------------------------------------------------
struct DropDownList : Control
{
	struct Item
	{
		string text;
		int value;
	};

	vector<Item> items;
	int selected_index;
};

//-----------------------------------------------------------------------------
struct Slider : Control
{
	int value, min_value, max_value, step;
};

//-----------------------------------------------------------------------------
struct DialogBox : Control
{
	struct Layout
	{
		Font* font;
		Texture* background;
		Color background_color, font_color;
		Int2 corners;
	};

	DialogBox() : layout(default_layout) {}
	DialogBox(Layout& layout) : layout(layout) {}
	void Draw() override;
	void Update(float dt) override;
	void OnEvent(int);

	Layout& layout;
	string text;
	Button button;
	Rect rect;

	static Layout default_layout;
};
