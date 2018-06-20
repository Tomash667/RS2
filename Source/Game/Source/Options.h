#pragma once

#include "GuiControls.h"

class Options : public Panel
{
public:
	Options(GameState* game_state);
	void Draw() override;
	void Update(float dt) override;
	void Show();

private:
	void OnEvent(int id);
	void Close();

	GameState* game_state;
	CheckBox* cb_fullscreen, *cb_vsync;
	DropDownList* ddl_resolution;
	Slider* sl_volume;
	int volume, resolution_index;
	bool fullscreen, vsync;
};
