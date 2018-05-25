#include "GameCore.h"
#include "MainMenu.h"
#include <GuiControls.h>
#include <ResourceManager.h>
#include <Gui.h>
#include "Version.h"

void MainMenu::Init(ResourceManager* res_mgr)
{
	Sprite* sprite = new Sprite;
	sprite->image = res_mgr->GetTexture("background.jpg");
	sprite->pos = Int2(0, 0);
	sprite->size = gui->GetWindowSize();
	Add(sprite);

	sprite = new Sprite;
	sprite->image = res_mgr->GetTexture("logo.png");
	sprite->pos = Int2(200, 200); // TODO
	sprite->size = Int2(256, 256);
	Add(sprite);

	Label* label = new Label;
	label->text = "v " VERSION_STR;
	label->pos = Int2(200, 556);
	label->size = Int2(200, 50);
	Add(label);
}
