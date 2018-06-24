#pragma once

//-----------------------------------------------------------------------------
enum GuiEvent
{
	G_MOVED,
	G_CHANGED_RESOLUTION
};

//-----------------------------------------------------------------------------
struct Control
{
	friend Container;
	friend Gui;

	Control() : visible(true), pos(Int2::Zero), global_pos(Int2::Zero), size(Int2::Zero), focus(false), parent(nullptr) {}
	virtual ~Control() {}
	virtual void Draw() {}
	virtual void Update(float dt) {}
	virtual void Event(GuiEvent event) {}

	void SetPos(const Int2& pos);

	const Int2& GetPos() const { return pos; }

private:
	Control* parent;
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
