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
		Int2 corners, pad;
		Color scroll_color, scroll_hover_color;
	};

	explicit ScrollBar(bool horizontal = false) : ScrollBar(default_layout, horizontal) {}
	explicit ScrollBar(Layout& layout, bool horizontal = false) : layout(layout), horizontal(horizontal), hover(HOVER_NONE), part(0), total(0), offset(0.f),
		clicked(false), click_step(10) {}
	void Draw() override;
	void Update(float dt) override;
	void SetExtent(int part, int total);
	void SetValue(float value) { offset = value; }
	void EndScrolling() { clicked = false; }

	bool IsScrolling() const { return clicked; }
	float GetValue() const { return offset; }

	int click_step;

private:
	enum Hover
	{
		HOVER_NONE,
		HOVER_ARROW_LESS,
		HOVER_ARROW_MORE,
		HOVER_SCROLL
	};

	void GetScrollPosSize(Int2& scroll_pos, Int2& scroll_size);

	Layout& layout;
	Hover hover;
	Int2 click_pt;
	int total, part;
	float offset;
	bool horizontal, clicked;

public:
	static Layout default_layout;
};

//-----------------------------------------------------------------------------
struct DropDownList : Control
{
	struct Layout
	{
		Texture* background;
		SpriteLayout<2> arrow;
		Int2 corners;
		Font* font;
		Color font_color, hover_color;
		int pad;
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
	int selected_index, hover_index, status, total_height, item_height;
	bool is_open, hover;

	static Layout default_layout;
};

//-----------------------------------------------------------------------------
struct ListBox : Control
{
	struct Layout
	{
		Texture* background;
		Int2 corners, pad;
		Font* font;
		Color font_color, selected_color;
	};

	struct Item
	{
		string text;
		int value;
	};

	ListBox(Layout& layout = default_layout) : layout(layout), selected_index(-1) {}
	void Init();
	void Draw() override;
	void Update(float dt) override;

	Layout& layout;
	vector<Item> items;
	int selected_index, item_height;

	static Layout default_layout;
};

//-----------------------------------------------------------------------------
struct TextBox : Control
{
	struct Layout
	{
		Texture* background;
		Int2 corners, pad;
		Font* font;
		Color font_color, color;
		int flags;
	};

	TextBox(Layout& layout = default_layout) : layout(layout) {}
	void Draw() override;

	Layout& layout;
	string text;

	static Layout default_layout;
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
