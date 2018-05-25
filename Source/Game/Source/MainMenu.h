#pragma once

#include <Control.h>

class MainMenu : public Container
{
public:

private:
	enum ButtonId
	{
		NEW_GAME,
		CONTINUE,
		EXIT
	};

	void Init(ResourceManager* res_mgr);
};
