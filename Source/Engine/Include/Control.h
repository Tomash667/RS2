#pragma once

//-----------------------------------------------------------------------------
enum GuiEvent
{
	G_MOVED
};

//-----------------------------------------------------------------------------
struct Control
{
	friend Container;

	Control() : visible(true), pos(Int2::Zero), global_pos(Int2::Zero), size(Int2::Zero), focus(false) {}
	virtual ~Control() {}
	virtual void Draw() {}
	virtual void Update(float dt) {}
	virtual void Event(GuiEvent event) {}

	void SetPos(const Int2& pos);

	const Int2& GetPos() const { return pos; }

protected:
	Int2 pos, global_pos;
public:
	Int2 size;
	bool visible, mouse_focus, focus;
	static Gui* gui;
};

//-----------------------------------------------------------------------------
struct Container : Control
{
	~Container();
	void Draw() override;
	void Update(float dt) override;
	void Event(GuiEvent event) override;
	void Add(Control* control);

protected:
	vector<Control*> controls;
};
