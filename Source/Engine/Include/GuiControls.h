#pragma once

#include "Control.h"

//-----------------------------------------------------------------------------
template<int N = 1>
struct SpriteLayout
{
	Box2d ToUV() const
	{
		return Box2d(0.f, 0.f, float(image_region.x) / image_size.x, float(image_region.y) / image_size.y);
	}

	Texture* image[N];
	Int2 image_size, image_region;
	Color color;
};

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
	struct Layout
	{
		Texture* image;
		Int2 corners;
		Color color;
	};

	Panel(Layout& layout = default_layout) : layout(layout) {}
	void Draw() override;

	Layout& layout;
	static Layout default_layout;
};

//-----------------------------------------------------------------------------
struct CustomPanel : Panel
{
	CustomPanel() : Panel(custom_layout) {}

	Panel::Layout custom_layout;
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

	Button(Layout& layout = default_layout) : layout(layout), id(0), event(nullptr), state(UP) {}
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
		Int2 size;
	};

	CheckBox(Layout& layout = default_layout) : layout(layout), checked(false), hover(false)
	{
		size = layout.size;
	}
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
		Texture* background;
		SpriteLayout<2> arrow;
		Int2 corners;
		Color scroll_color, scroll_hover_color;
	};

	explicit ScrollBar(bool horizontal = false) : ScrollBar(default_layout) {}
	explicit ScrollBar(Layout& layout, bool horizontal = false) : layout(layout), horizontal(horizontal), hover(HOVER_NONE) {}
	void Draw() override;
	void Update(float dt) override;

private:
	enum Hover
	{
		HOVER_NONE,
		HOVER_ARROW_LESS,
		HOVER_ARROW_MORE,
		HOVER_SCROLL
	};

	Layout& layout;
	Hover hover;
	int total, available_region;
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
		SpriteLayout arrow;
		Int2 corners;
		Font* font;
		Color font_color;
		int pad, item_pad;
	};

	struct Item
	{
		string text;
		int value;
	};

	DropDownList(Layout& layout = default_layout) : layout(layout), selected_index(-1), is_open(false), hover(false) {}
	void Init();
	void Draw() override;
	void Update(float dt) override;

	Layout& layout;
	vector<Item> items;
	int selected_index, hover_index, status, total_height;
	bool is_open, hover;

	static Layout default_layout;
};

//-----------------------------------------------------------------------------
struct Slider : Control
{
	int value, max_value, step;
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

	DialogBox(Layout& layout = default_layout) : layout(layout) {}
	void Draw() override;
	void Update(float dt) override;
	void OnEvent(int);

	Layout& layout;
	string text;
	Button button;
	Rect rect;

	static Layout default_layout;
};
