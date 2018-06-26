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
	sprite_background = new Sprite;
	sprite_background->image = res_mgr->GetTexture("gui/background.jpg");
	Add(sprite_background);

	// logo
	sprite_logo = new Sprite;
	sprite_logo->image = res_mgr->GetTexture("gui/logo.png");
	sprite_logo->size = Int2(512, 256);
	Add(sprite_logo);

	// version label
	lab_version = new Label;
	lab_version->text = "version " VERSION_STR;
	lab_version->size = Int2(512, 50);
	lab_version->color = Color(255, 0, 0);
	lab_version->flags = Font::Center;
	Add(lab_version);

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

	// options
	options = new Options(game_state);

	// show
	PositionControls();
	gui->Add(this);
	Show();
}

void MainMenu::Event(GuiEvent event)
{
	if(event == G_CHANGED_RESOLUTION)
		PositionControls();
}

void MainMenu::PositionControls()
{
	size = gui->GetWindowSize();
	sprite_background->size = size;

	sprite_logo->SetPos(Int2((size.x - 200 - sprite_logo->size.x) / 2, 200));
	lab_version->SetPos(Int2(sprite_logo->GetPos().x, 200 + 180));

	int part = size.x / BUTTON_MAX;
	int pos_y = size.y - buttons[0]->size.y * 2;

	for(int i = 0; i < BUTTON_MAX; ++i)
		buttons[i]->SetPos(Int2(part * i + (part - buttons[0]->size.x) / 2, pos_y));
}

void MainMenu::InitLayout(ResourceManager* res_mgr)
{
	Panel::default_layout.image = res_mgr->GetTexture("gui/panel.png");
	Panel::default_layout.corners = Int2(6, 32);
	Panel::default_layout.color = Color(255, 255, 255, 200);

	Button::default_layout.image = res_mgr->GetTexture("gui/button.png");
	Button::default_layout.image_hover = res_mgr->GetTexture("gui/button_hover.png");
	Button::default_layout.image_disabled = res_mgr->GetTexture("gui/button_disabled.png");
	Button::default_layout.font_color = Color(0, 255, 33);
	Button::default_layout.font_color_disabled = Color(50, 50, 50);
	Button::default_layout.corners = Int2(6, 32);

	CheckBox::default_layout.background = res_mgr->GetTexture("gui/checkbox.png");
	CheckBox::default_layout.hover = res_mgr->GetTexture("gui/checkbox_hover.png");
	CheckBox::default_layout.checkbox = res_mgr->GetTexture("gui/checked.png");
	CheckBox::default_layout.size = Int2(32, 32);

	ScrollBar::default_layout.arrow.image[0] = res_mgr->GetTexture("gui/scroll_arrow.png");
	ScrollBar::default_layout.arrow.image[1] = res_mgr->GetTexture("gui/scroll_arrow_hover.png");
	ScrollBar::default_layout.arrow.color = Color::White;
	ScrollBar::default_layout.arrow.image_size = Int2(16, 16);
	ScrollBar::default_layout.arrow.image_region = Int2(9, 13);
	ScrollBar::default_layout.background = res_mgr->GetTexture("gui/scrollbar.png");
	ScrollBar::default_layout.corners = Int2(2, 8);
	ScrollBar::default_layout.scroll_color = Color(38, 127, 0, 255);
	ScrollBar::default_layout.scroll_hover_color = Color(1, 248, 31, 255);
	ScrollBar::default_layout.pad = Int2(1, 2);

	DropDownList::default_layout.arrow.image[0] = res_mgr->GetTexture("gui/dropdownlist_arrow.png");
	DropDownList::default_layout.arrow.image[1] = res_mgr->GetTexture("gui/dropdownlist_arrow_hover.png");
	DropDownList::default_layout.arrow.image_size = Int2(16, 8);
	DropDownList::default_layout.arrow.image_region = Int2(15, 7);
	DropDownList::default_layout.arrow.color = Color::White;
	DropDownList::default_layout.background = res_mgr->GetTexture("gui/scrollbar.png");
	DropDownList::default_layout.corners = Int2(2, 8);
	DropDownList::default_layout.font_color = Color(0, 255, 33);
	DropDownList::default_layout.pad = 2;
	DropDownList::default_layout.hover_color = Color(0, 255, 33, 128);

	ListBox::default_layout.background = res_mgr->GetTexture("gui/scrollbar.png");
	ListBox::default_layout.corners = Int2(2, 8);
	ListBox::default_layout.font_color = Color(0, 255, 33);
	ListBox::default_layout.pad = Int2(8, 4);
	ListBox::default_layout.selected_color = Color(0, 255, 33, 128);

	TextBox::default_layout.background = res_mgr->GetTexture("gui/scrollbar.png");
	TextBox::default_layout.corners = Int2(2, 8);
	TextBox::default_layout.font_color = Color(0, 255, 33);
	TextBox::default_layout.pad = Int2(4, 1);
	TextBox::default_layout.flags = Font::VCenter;
	TextBox::default_layout.color = Color::White;

	DialogBox::default_layout.background = res_mgr->GetTexture("gui/panel.png");
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
	if(mouse_focus && gui->GetInput()->Pressed(Key::Escape))
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
