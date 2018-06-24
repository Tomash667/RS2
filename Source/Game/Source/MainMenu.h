#pragma once

#include <Control.h>

class MainMenu : public Container
{
public:
	MainMenu();
	~MainMenu();
	void Init(ResourceManager* res_mgr, GameState* game_state);
	void Event(GuiEvent event) override;
	void Show();
	void Hide();

	Options* options;

private:
	enum ButtonId
	{
		CONTINUE,
		NEW_GAME,
		OPTIONS,
		EXIT,
		BUTTON_MAX
	};

	void InitLayout(ResourceManager* res_mgr);
	void PositionControls();
	void Update(float dt) override;
	void OnEvent(int id);

	GameState* game_state;
	Button* buttons[BUTTON_MAX];
	Sprite* sprite_background, *sprite_logo;
	Label* lab_version;
};
