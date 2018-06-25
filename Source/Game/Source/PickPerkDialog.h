#pragma once

#include "GuiControls.h"

class PickPerkDialog : public Panel
{
public:
	PickPerkDialog(GameState* game_state, ResourceManager* res_mgr);
	void Update(float dt) override;
	void Show(Player* player);

private:
	void OnEvent(int id);

	GameState* game_state;
	Player* player;
	ListBox* list_perks;
	TextBox* textbox_desc;
	Button* bt;
	vector<std::pair<PerkId, int>> perks;
	int last_index;
};
