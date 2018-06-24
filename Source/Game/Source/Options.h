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
	ScrollBar* scroll_volume;
	Label* lab_volume;
	Sound* sound_test;
	int volume, resolution_index;
	float last_sound_test;
	bool fullscreen, vsync;
};
