#pragma once

#include "GuiControls.h"

class StatsPanel : public Panel
{
public:
	StatsPanel(ResourceManager* res_mgr);
	void Show(Player* player);
	void Update(float dt) override;

private:
	void UpdateStats();
	void UpdatePerks();

	Player* player;
	TextBox* textbox_stats, *textbox_perks;
	TextBox::Layout textbox_layout;
	float timer;
};
