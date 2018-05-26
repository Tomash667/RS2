#include "GameCore.h"
#include "MainMenu.h"
#include <GuiControls.h>
#include <ResourceManager.h>
#include <Gui.h>
#include <Font.h>
#include <Input.h>
#include "Version.h"
#include "GameState.h"

void MainMenu::Init(ResourceManager* res_mgr, GameState* game_state)
{
	this->game_state = game_state;

	const Int2& wnd_size = gui->GetWindowSize();

	Sprite* sprite = new Sprite;
	sprite->image = res_mgr->GetTexture("background.jpg");
	sprite->pos = Int2(0, 0);
	sprite->size = gui->GetWindowSize();
	Add(sprite);

	sprite = new Sprite;
	sprite->image = res_mgr->GetTexture("logo.png");
	sprite->pos = Int2(200, 200);
	sprite->size = Int2(512, 256);
	Add(sprite);

	Label* label = new Label;
	label->text = "version " VERSION_STR;
	label->pos = Int2(200, 200 + 180);
	label->size = Int2(512, 50);
	label->color = Color(255, 0, 0);
	label->flags = Font::Center;
	Add(label);

	Button::default_layout.image = res_mgr->GetTexture("button.png");
	Button::default_layout.image_hover = res_mgr->GetTexture("button_hover.png");
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
	Button::NormalizeSize(buttons, BUTTON_MAX, Int2(10, 10));

	int part = wnd_size.x / 3;
	int pos_y = wnd_size.y - buttons[0]->size.y * 2;

	for(int i = 0; i < 3; ++i)
		buttons[i]->pos = Int2(part * i + (part - buttons[0]->size.x) / 2, pos_y);

	buttons[0]->state = Button::DISABLED;

	gui->Add(this);
	gui->SetCursorVisible(true);
}

void MainMenu::Show()
{
	visible = true;
	gui->SetCursorVisible(true);
}

void MainMenu::Hide()
{
	visible = false;
	gui->SetCursorVisible(false);
}

void MainMenu::Update(float dt)
{
	if(gui->GetInput()->Pressed(Key::Escape))
		game_state->SetChangeState(GameState::QUIT);
	else
		Container::Update(dt);
}

void MainMenu::OnEvent(int id)
{
	switch(id)
	{
	case CONTINUE:
		game_state->SetChangeState(GameState::CONTINUE);
		break;
	case NEW_GAME:
		game_state->SetChangeState(GameState::NEW_GAME);
		break;
	case EXIT:
		game_state->SetChangeState(GameState::QUIT);
		break;
	}
}
