#pragma once

#include <Control.h>

class GameGui : public Container
{
public:
	GameGui();
	~GameGui();
	void Init(Engine* engine, Player* player);
	void Draw() override;
	void Update(float dt) override;

	bool IsInventoryOpen();

private:
	void DrawCrosshair(int size, int dist, int length);

	Engine* engine;
	Player* player;
	Inventory* inventory;
	Panel* panel_fps;
	Label* label_fps, *label_medkits, *label_ammo;
	Sprite* sprite_crosshair, *sprite_food;
	ProgressBar* hp_bar;
};
