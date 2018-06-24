#include "EngineCore.h"
#include "Control.h"
#include "Gui.h"


Gui* Control::gui;


void Control::SetPos(const Int2& new_pos)
{
	if(pos == new_pos)
		return;
	Int2 dif = global_pos - pos;
	pos = new_pos;
	global_pos = pos + dif;
	Event(G_MOVED);
}


Container::~Container()
{
	DeleteElements(controls);
}

void Container::Draw()
{
	for(Control* control : controls)
	{
		if(control->visible)
			control->Draw();
	}
}

void Container::Add(Control* control)
{
	assert(control && !control->parent);
	controls.push_back(control);
	control->parent = this;
	control->global_pos = global_pos + control->pos;
}

void Container::Update(float dt)
{
	for(Control* control : controls)
	{
		if(control->visible)
		{
			control->mouse_focus = mouse_focus;
			control->Update(dt);
		}
	}
}

void Container::Event(GuiEvent event)
{
	if(event == G_MOVED)
	{
		for(Control* control : controls)
		{
			control->global_pos = global_pos + control->pos;
			control->Event(G_MOVED);
		}
	}
	else
	{
		for(Control* control : controls)
			control->Event(G_CHANGED_RESOLUTION);
	}
}
