#pragma once

//-----------------------------------------------------------------------------
struct Control
{
	Control() : visible(true), pos(Int2::Zero), size(Int2::Zero), focus(false) {}
	virtual ~Control() {}
	virtual void Draw() {}
	virtual void Update(float dt) {}

	Int2 pos, size;
	bool visible, mouse_focus, focus;
	static Gui* gui;
};

//-----------------------------------------------------------------------------
struct Container : Control
{
	~Container();
	void Draw() override;
	void Update(float dt) override;
	void Add(Control* control);

protected:
	vector<Control*> controls;
};
