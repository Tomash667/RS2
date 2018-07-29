#pragma once

#include <Control.h>

struct SpeechBubble
{
	Unit* unit;
	string text;
};

class GameGui : public Container
{
public:
	GameGui();
	~GameGui();
	void Init(Engine* engine, GameState* game_state, Options* options);
	void Draw() override;
	void Update(float dt) override;
	void Event(GuiEvent event) override;
	void AddBubble(cstring text, Unit* unit);

	bool IsMouseRequired();

private:
	void PositionControls();
	void DrawLabel(cstring text, const Vec3& pos);
	void DrawCrosshair(int size, int dist, int length);

	Engine* engine;
	GameState* game_state;
	Options* options;
	Inventory* inventory;
	StatsPanel* stats_panel;
	Panel* panel_fps;
	Label* label_fps, *label_medkits, *label_ammo;
	Sprite* sprite_crosshair, *sprite_food;
	ProgressBar* hp_bar;
	Texture* tex_background;
	Font* font_big;
	float death_timer;
	vector<SpeechBubble*> bubbles; // FIXME - save/load/clear
};
