#include "EngineCore.h"
#include "Control.h"


Gui* Control::gui;


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
	assert(control);
	controls.push_back(control);
}

void Container::Update(float dt)
{
	for(Control* control : controls)
		control->Update(dt);
}
