#include "GameCore.h"
#include "MainMenu.h"
#include <GuiControls.h>
#include <ResourceManager.h>
#include <Gui.h>
#include "Version.h"
#include "GameState.h"

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

	Button::default_layout.image = res_mgr->GetTexture("button.png");
	Button::default_layout.image_hover = res_mgr->GetTexture("button_hover.png");
	Button::default_layout.image_down = res_mgr->GetTexture("button_down.png");
	Button::default_layout.image_disabled = res_mgr->GetTexture("button_disabled.png");
	Button::default_layout.font_color = Color(0, 255, 33);
	Button::default_layout.font_color_disabled = Color(50, 50, 50);
	Button::default_layout.corners = Int2(6, 32);

	cstring texts[BUTTON_MAX] = {
		"Continue",
		"New game",
		"Exit"
	};

	for(uint i = 0; i < BUTTON_MAX; ++i)
	{
		Button* bt = new Button;
		bt->id = i;
		bt->event = delegate<void(int)>(this, &MainMenu::OnEvent);
		bt->text = texts[i];
		buttons[i] = bt;
		Add(bt);
	}
	Button::NormalizeSize(buttons, BUTTON_MAX, Int2(4, 4));

	buttons[0]->pos = Int2(100, 300);
	buttons[1]->pos = Int2(100, 350);
	buttons[2]->pos = Int2(100, 400);

	buttons[0]->state = Button::DISABLED;

	gui->Add(this);
}

void MainMenu::OnEvent(int id)
{
	switch(id)
	{
	case CONTINUE:
		global::state.SetChangeState(GameState::CONTINUE);
		break;
	case NEW_GAME:
		global::state.SetChangeState(GameState::NEW_GAME);
		break;
	case EXIT:
		global::state.SetChangeState(GameState::QUIT);
		break;
	}
}
