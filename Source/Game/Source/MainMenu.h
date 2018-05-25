#pragma once

#include <Control.h>

class MainMenu : public Container
{
public:
	void Init(ResourceManager* res_mgr);

private:
	enum ButtonId
	{
		CONTINUE,
		NEW_GAME,
		EXIT,
		BUTTON_MAX
	};

	void OnEvent(int id);

	Button* buttons[BUTTON_MAX];
};
