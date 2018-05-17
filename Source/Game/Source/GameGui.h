#pragma once

#include <Control.h>

class GameGui : public Container
{
public:
	void Init(Engine* engine, Player* player);
	void Draw() override;
	void Update();

	bool IsInventoryOpen();

private:
	Engine* engine;
	Player* player;
	Inventory* inventory;
	Panel* panel_fps;
	Label* label_fps, *label_medkits;
	ProgressBar* hp_bar;
};
