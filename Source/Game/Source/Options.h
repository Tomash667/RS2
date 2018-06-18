#pragma once

#include "GuiControls.h"

class Options : public Panel
{
public:
	Options(GameState* state);
	void Draw() override;
	void Update(float dt) override;
	void Show();

private:
	enum EventId
	{
		ACCEPT,
		CANCEL
	};

	void OnEvent(int id);

	GameState* state;
	CheckBox* cb_fullscreen, *cb_vsync;
	DropDownList* ddl_resolution;
	Slider* sl_volume;
};
