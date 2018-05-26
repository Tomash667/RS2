#pragma once

#include <Control.h>

class MainMenu : public Container
{
public:
	void Init(ResourceManager* res_mgr, GameState* game_state);
	void Show();
	void Hide();

private:
	enum ButtonId
	{
		CONTINUE,
		NEW_GAME,
		EXIT,
		BUTTON_MAX
	};

	void Update(float dt) override;
	void OnEvent(int id);

	GameState* game_state;
	Button* buttons[BUTTON_MAX];
};
