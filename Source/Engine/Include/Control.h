#pragma once

//-----------------------------------------------------------------------------
struct Control
{
	Control() : visible(true), pos(Int2::Zero), size(Int2::Zero) {}
	virtual ~Control() {}
	virtual void Draw() {}

	Int2 pos, size;
	bool visible;
	static Gui* gui;
};

//-----------------------------------------------------------------------------
struct Container : Control
{
	~Container();
	void Draw() override;
	void Add(Control* control);

protected:
	vector<Control*> controls;
};
