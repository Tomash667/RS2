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

	Button() : Button(default_layout) {}
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
	struct Layout
	{
		Texture* background, *hover, *checkbox;
	};

	CheckBox() : CheckBox(default_layout) {}
	CheckBox(Layout& layout) : layout(layout), checked(false), hover(false) {}
	void Draw() override;
	void Update(float dt) override;

	Layout& layout;
	bool checked, hover;

	static Layout default_layout;
};

//-----------------------------------------------------------------------------
struct ScrollBar : Control
{
	struct Layout
	{
		Texture* background, *arrow, *arrow_hover;
		Int2 corners, arrow_size, arrow_image_size;
		Color scroll_color, scroll_hover_color;
	};

	explicit ScrollBar(bool horizontal = false) : ScrollBar(default_layout) {}
	explicit ScrollBar(Layout& layout, bool horizontal = false) : layout(layout), horizontal(horizontal), hover(0) {}
	void Draw() override;
	void Update(float dt) override;

private:
	Layout& layout;
	int hover;
	bool horizontal;

public:
	static Layout default_layout;
};

//-----------------------------------------------------------------------------
struct DropDownList : Control
{
	struct Layout
	{
		Texture* background, *background_hover, *list_background, *list_hover;
		Int2 corners;
		Font* font;
		Color font_color;
	};

	struct Item
	{
		string text;
		int value;
	};

	DropDownList() : DropDownList(default_layout) {}
	DropDownList(Layout& layout) : layout(layout), selected_index(-1), is_open(false), hover(false) {}
	void Draw() override;
	void Update(float dt) override;

	Layout& layout;
	vector<Item> items;
	int selected_index, hover_index, status;
	bool is_open, hover;

	static Layout default_layout;
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
