#include "GameCore.h"
#include "MainMenu.h"
#include <GuiControls.h>
#include <ResourceManager.h>
#include <Gui.h>
#include <Font.h>
#include <Input.h>
#include "Version.h"
#include "GameState.h"
#include "Options.h"

MainMenu::MainMenu() : options(nullptr)
{
}

MainMenu::~MainMenu()
{
	delete options;
}

void MainMenu::Init(ResourceManager* res_mgr, GameState* game_state)
{
	this->game_state = game_state;

	InitLayout(res_mgr);

	// background
	Sprite* sprite = new Sprite;
	sprite->image = res_mgr->GetTexture("background.jpg");
	sprite->size = gui->GetWindowSize();
	Add(sprite);

	// logo
	sprite = new Sprite;
	sprite->image = res_mgr->GetTexture("logo.png");
	sprite->SetPos(Int2(200, 200));
	sprite->size = Int2(512, 256);
	Add(sprite);

	// version label
	Label* label = new Label;
	label->text = "version " VERSION_STR;
	label->SetPos(Int2(200, 200 + 180));
	label->size = Int2(512, 50);
	label->color = Color(255, 0, 0);
	label->flags = Font::Center;
	Add(label);

	// buttons
	cstring texts[BUTTON_MAX] = {
		"Continue",
		"New game",
		"Options",
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

	const Int2& wnd_size = gui->GetWindowSize();
	int part = wnd_size.x / BUTTON_MAX;
	int pos_y = wnd_size.y - buttons[0]->size.y * 2;

	for(int i = 0; i < BUTTON_MAX; ++i)
		buttons[i]->SetPos(Int2(part * i + (part - buttons[0]->size.x) / 2, pos_y));

	// options
	options = new Options(game_state);

	// show
	gui->Add(this);
	Show();
}

void MainMenu::InitLayout(ResourceManager* res_mgr)
{
	Panel::default_layout.image = res_mgr->GetTexture("panel.png");
	Panel::default_layout.corners = Int2(6, 32);
	Panel::default_layout.color = Color(255, 255, 255, 200);

	Button::default_layout.image = res_mgr->GetTexture("button.png");
	Button::default_layout.image_hover = res_mgr->GetTexture("button_hover.png");
	Button::default_layout.image_disabled = res_mgr->GetTexture("button_disabled.png");
	Button::default_layout.font_color = Color(0, 255, 33);
	Button::default_layout.font_color_disabled = Color(50, 50, 50);
	Button::default_layout.corners = Int2(6, 32);

	CheckBox::default_layout.background = res_mgr->GetTexture("checbox.png");
	CheckBox::default_layout.hover = res_mgr->GetTexture("checbox_hover.png");
	CheckBox::default_layout.checkbox = res_mgr->GetTexture("checked.png");
	CheckBox::default_layout.size = Int2(32, 32);

	ScrollBar::default_layout.arrow.image = res_mgr->GetTexture("scroll_arrow.png");
	ScrollBar::default_layout.arrow.color = Color::White;
	ScrollBar::default_layout.arrow.image_size = Int2(16, 16);
	ScrollBar::default_layout.arrow.image_region = Int2(8, 12);
	ScrollBar::default_layout.background = res_mgr->GetTexture("scrollbar.png");
	ScrollBar::default_layout.corners = Int2(2, 16);
	ScrollBar::default_layout.scroll_color = Color(128, 0, 0, 255); // FIXME
	ScrollBar::default_layout.scroll_hover_color = Color(255, 0, 0, 255); // FIXME

	DialogBox::default_layout.background = res_mgr->GetTexture("panel.png");
	DialogBox::default_layout.corners = Int2(6, 32);
	DialogBox::default_layout.background_color = Color(255, 255, 255, 200);
	DialogBox::default_layout.font_color = Color(0, 255, 33);
}

void MainMenu::Show()
{
	visible = true;
	gui->SetCursorVisible(true);
	buttons[0]->state = (io::FileExists("save") ? Button::UP : Button::DISABLED);
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
	case OPTIONS:
		options->Show();
		break;
	case EXIT:
		game_state->SetChangeState(GameState::QUIT);
		break;
	}
}
