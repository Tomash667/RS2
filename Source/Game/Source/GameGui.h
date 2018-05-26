#pragma once

#include <Control.h>

class GameGui : public Container
{
public:
	GameGui();
	~GameGui();
	void Init(Engine* engine, GameState* game_state);
	void Draw() override;
	void Update(float dt) override;

	bool IsInventoryOpen();

private:
	void DrawCrosshair(int size, int dist, int length);

	Engine* engine;
	GameState* game_state;
	Inventory* inventory;
	Panel* panel_fps;
	Label* label_fps, *label_medkits, *label_ammo;
	Sprite* sprite_crosshair, *sprite_food;
	ProgressBar* hp_bar;
	Texture* tex_background;
	Font* font_big;
	float death_timer;
};
