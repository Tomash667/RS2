#include "GameCore.h"
#include "GameGui.h"
#include "Player.h"
#include <Engine.h>
#include <Gui.h>
#include <GuiControls.h>
#include <ResourceManager.h>
#include <Input.h>
#include <SceneNode.h>
#include "GroundItem.h"
#include "Item.h"
#include "Inventory.h"
#include <Font.h>
#include "GameState.h"
#include "Options.h"
#include "StatsPanel.h"


enum DIR
{
	DIR_N,
	DIR_S,
	DIR_E,
	DIR_W,
	DIR_NE,
	DIR_NW,
	DIR_SE,
	DIR_SW
};

cstring dir_name[] = {
	"N",
	"S",
	"E",
	"W",
	"NE",
	"NW",
	"SE",
	"SW"
};

DIR AngleToDir(float angle)
{
	assert(InRange(angle, 0.f, 2 * PI));
	if(angle < 1.f / 8 * PI)
		return DIR_E;
	else if(angle < 3.f / 8 * PI)
		return DIR_NE;
	else if(angle < 5.f / 8 * PI)
		return DIR_N;
	else if(angle < 7.f / 8 * PI)
		return DIR_NW;
	else if(angle < 9.f / 8 * PI)
		return DIR_W;
	else if(angle < 11.f / 8 * PI)
		return DIR_SW;
	else if(angle < 13.f / 8 * PI)
		return DIR_S;
	else if(angle < 15.f / 8 * PI)
		return DIR_SE;
	else
		return DIR_E;
}


GameGui::GameGui() : sprite_crosshair(nullptr), stats_panel(nullptr)
{
}

GameGui::~GameGui()
{
	delete sprite_crosshair;
	delete stats_panel;
}

void GameGui::Init(Engine* engine, GameState* game_state, Options* options)
{
	this->engine = engine;
	this->game_state = game_state;
	this->options = options;

	const Int2& wnd_size = gui->GetWindowSize();
	ResourceManager* res_mgr = engine->GetResourceManager();

	gui->SetCursorTexture(res_mgr->GetTexture("gui/cursor.png"));

	// crosshair
	sprite_crosshair = new Sprite;
	sprite_crosshair->image = res_mgr->GetTexture("gui/crosshair_dot.png");
	sprite_crosshair->size = Int2(16, 16);
	sprite_crosshair->SetPos((wnd_size - sprite_crosshair->size) / 2);

	// hp bar
	hp_bar = new ProgressBar;
	hp_bar->image = res_mgr->GetTexture("gui/hp_bar.png");
	hp_bar->background = res_mgr->GetTexture("gui/bar_empty.png");
	hp_bar->size = Int2(256, 30);
	hp_bar->SetPos(Int2(0, wnd_size.y - hp_bar->size.y));
	Add(hp_bar);

	// food icon
	sprite_food = new Sprite;
	sprite_food->image = res_mgr->GetTexture("gui/food_icon.png");
	sprite_food->size = Int2(32, 32);
	sprite_food->SetPos(Int2(8, hp_bar->GetPos().y - 40));
	sprite_food->visible = false;
	Add(sprite_food);

	// panel with medkits/ammo
	Panel* panel_bg = new Panel;
	panel_bg->SetPos(Int2(260, wnd_size.y - 36));
	panel_bg->size = Int2(200, 80);
	Add(panel_bg);

	// medkit icon & counter
	Sprite* sprite_medkit = new Sprite;
	sprite_medkit->image = res_mgr->GetTexture("gui/medkit_icon.png");
	sprite_medkit->size = Int2(32, 32);
	sprite_medkit->SetPos(Int2(6, 4));
	panel_bg->Add(sprite_medkit);

	label_medkits = new Label;
	label_medkits->font = res_mgr->GetFont("Arial", 20);
	label_medkits->SetPos(sprite_medkit->GetPos() + Int2(32, 2));
	label_medkits->size = Int2(100, 100);
	label_medkits->color = Color(0, 255, 33);
	panel_bg->Add(label_medkits);

	// ammo counter
	Sprite* sprite_ammo = new Sprite;
	sprite_ammo->image = res_mgr->GetTexture("gui/ammo.png");
	sprite_ammo->SetPos(Int2(label_medkits->GetPos() + Int2(32, -2)));
	sprite_ammo->size = Int2(32, 32);
	panel_bg->Add(sprite_ammo);

	label_ammo = new Label;
	label_ammo->font = label_medkits->font;
	label_ammo->SetPos(Int2(sprite_ammo->GetPos() + Int2(32, 2)));
	label_ammo->size = Int2(100, 32);
	label_ammo->color = Color(0, 255, 33);
	label_ammo->visible = false;
	panel_bg->Add(label_ammo);

	// fps panel
	label_fps = new Label;
	label_fps->SetPos(Int2(6, 6));
	label_fps->color = Color(0, 255, 33, 255);
	panel_fps = new Panel;
	panel_fps->visible = false;
	panel_fps->Add(label_fps);
	Add(panel_fps);

	// inventory
	inventory = new Inventory(res_mgr, game_state);
	Add(inventory);

	// stats panel
	stats_panel = new StatsPanel(res_mgr);

	tex_background = res_mgr->GetTexture("gui/background.png");
	res_mgr->AddFontFromFile("fonts/pertili.ttf");
	font_big = res_mgr->GetFont("Perpetua Titling MT", 40);

	PositionControls();
	engine->GetGui()->Add(this);
}

void GameGui::PositionControls()
{
	const Int2& wnd_size = gui->GetWindowSize();
	inventory->SetPos(Int2(wnd_size.x - inventory->size.x, wnd_size.y - inventory->size.y));
	sprite_crosshair->SetPos((wnd_size - sprite_crosshair->size) / 2);
}

void GameGui::Draw()
{
	Container::Draw();

	Player* player = game_state->player;

	if(player->item_before)
	{
		Vec3 item_pos = player->item_before->pos;
		item_pos.y += 0.5f;
		cstring text = Format("[E] %s", player->item_before->item->name);
		DrawLabel(text, item_pos);
	}

	if(player->unit_before)
	{
		Vec3 pos = player->unit_before->node->pos;
		pos.y += Unit::height;
		DrawLabel("[E] Talk", pos);
	}

	if(death_timer > 1.f)
	{
		// death screen
		int alpha = (int)(Min(1.f, (death_timer - 1.f)) * 255);
		Color front = Color(255, 0, 0, alpha);
		Color back = Color(0, 0, 0, alpha);
		gui->DrawSprite(nullptr, Int2::Zero, gui->GetWindowSize(), Color(50, 50, 50, alpha * 150 / 255));
		gui->DrawTextOutline("YOU DIED", font_big, front, back, Font::Center | Font::VCenter, Rect::Create(Int2::Zero, gui->GetWindowSize()));
		gui->DrawTextOutline("Esc to exit to menu", nullptr, front, back, Font::Center | Font::VCenter,
			Rect::Create(Int2(0, 100), gui->GetWindowSize()));
	}
	else if(game_state->IsPaused())
	{
		// paused
		if(!gui->HaveDialog())
		{
			gui->DrawSprite(nullptr, Int2::Zero, gui->GetWindowSize(), Color(50, 50, 50, 150));
			gui->DrawTextOutline("GAME PAUSED", font_big, Color(0, 255, 33), Color::Black, Font::Center | Font::VCenter,
				Rect::Create(Int2::Zero, gui->GetWindowSize()));
			gui->DrawTextOutline("Esc to continue, Enter to save & quit, O for options", nullptr, Color(0, 255, 33), Color::Black, Font::Center | Font::VCenter,
				Rect::Create(Int2(0, 100), gui->GetWindowSize()));
		}
	}
	else
	{
		// cursor
		if(!inventory->visible)
		{
			if(player->action == A_AIM)
				DrawCrosshair(4, (int)player->aim, 20);
			else
				sprite_crosshair->Draw();
		}
	}
}

void GameGui::DrawLabel(cstring text, const Vec3& pos)
{
	Int2 text_pos;
	if(!gui->To2dPoint(pos, text_pos))
		return;

	const Int2& wnd_size = gui->GetWindowSize();
	Int2 text_size = gui->GetDefaultFont()->CalculateSize(text) + Int2(2, 2);
	text_pos -= text_size / 2;
	if(text_pos.x < 0)
		text_pos.x = 0;
	else if(text_pos.x + text_size.x >= wnd_size.x)
		text_pos.x = wnd_size.x - text_size.x;
	if(text_pos.y < 0)
		text_pos.y = 0;
	else if(text_pos.y + text_size.y >= wnd_size.y)
		text_pos.y = wnd_size.y - text_size.y;

	gui->DrawSprite(nullptr, text_pos, text_size, Color(0, 163, 33, 128));
	gui->DrawText(text, nullptr, Color::Black, Font::Top | Font::Center, Rect::Create(text_pos, text_size));
}

void GameGui::Update(float dt)
{
	Input* input = gui->GetInput();
	Player* player = game_state->player;

	// fps panel
	if(mouse_focus && input->Pressed(Key::F1))
		panel_fps->visible = !panel_fps->visible;
	if(panel_fps->visible)
	{
#ifdef _DEBUG
		label_fps->text = Format("Fps: %g\nPos: %g; %g; %g\nRot: %g %s\nTime: %02d:%02d", FLT10(engine->GetFps()),
			FLT10(player->node->pos.x), FLT10(player->node->pos.y), FLT10(player->node->pos.z),
			FLT10(player->node->rot.y), dir_name[AngleToDir(player->node->rot.y)],
			(int)floor(game_state->hour), int((game_state->hour - floor(game_state->hour)) * 60));
#else
		label_fps->text = Format("Fps: %g", FLT10(engine->GetFps()));
#endif
		label_fps->size = label_fps->CalculateSize();
		Int2 panel_size = label_fps->size + Int2(2 * panel_fps->layout.corners.x, 2 * panel_fps->layout.corners.x);
		if(panel_size > panel_fps->size)
			panel_fps->size = Int2::Max(panel_size, panel_fps->size);
	}
	bool have_dialog = false;
	if(!mouse_focus)
	{
		if(stats_panel->visible)
			have_dialog = true;
		else
			return;
	}

	// hp bar
	hp_bar->progress = player->GetHpp();

	// medkits counter
	label_medkits->text = Format("%d", player->medkits);

	// ammo counter
	if(player->use_melee)
		label_ammo->visible = false;
	else
	{
		label_ammo->text = Format("%u/%u", player->current_ammo, player->ammo);
		label_ammo->visible = true;
	}

	// food icon
	FoodLevel food_level = player->GetFoodLevel();
	if(food_level == FL_NORMAL)
		sprite_food->visible = false;
	else
	{
		sprite_food->visible = true;
		switch(food_level)
		{
		case FL_FULL:
			sprite_food->color = Color(0, 255, 33);
			break;
		case FL_HUNGRY:
			sprite_food->color = Color(240, 216, 0);
			break;
		case FL_VERY_HUGRY:
			sprite_food->color = Color(245, 150, 0);
			break;
		case FL_STARVING:
			sprite_food->color = Color(255, 0, 0);
			break;
		}
	}

	if(player->hp > 0)
		death_timer = 0.f;
	else
		death_timer += dt;

	// show/hide inventory
	if(player->hp > 0)
	{
		if(input->Pressed(Key::I))
		{
			if(stats_panel->visible)
				gui->CloseDialog();
			inventory->Show(!inventory->visible);
		}
		else if(input->Pressed(Key::K))
		{
			if(stats_panel->visible)
				gui->CloseDialog();
			else
			{
				if(inventory->visible)
					inventory->Show(false);
				stats_panel->Show(player);
			}
		}
	}
	else
	{
		if(inventory->visible)
			inventory->Show(false);
		if(stats_panel->visible)
			gui->CloseDialog();
	}

	if(!have_dialog)
	{
		if(inventory->visible)
			inventory->Update(dt);
		else if(game_state->IsPaused())
		{
			if(input->Pressed(Key::Enter))
				game_state->SetChangeState(GameState::SAVE_AND_EXIT);
			else if(input->Pressed(Key::Escape))
				game_state->SetPaused(false);
			else if(input->Pressed(Key::O))
				options->Show();
		}
		else if(input->Pressed(Key::Escape))
		{
			if(death_timer == 0.f)
				game_state->SetPaused(true);
			else if(death_timer > 2.f)
				game_state->SetChangeState(GameState::EXIT_TO_MENU);
		}
	}
}

void GameGui::Event(GuiEvent event)
{
	if(event == G_CHANGED_RESOLUTION)
		PositionControls();
}

bool GameGui::IsMouseRequired()
{
	return inventory->visible || stats_panel->visible;
}

void GameGui::DrawCrosshair(int size, int dist, int length)
{
	Color col(0, 255, 33);
	Int2 center = gui->GetWindowSize() / 2;
	gui->DrawSprite(nullptr, Int2(center.x - size / 2 - length - dist + 1, center.y - size / 2 + 1), Int2(length - 1, size - 1), Color::Black);
	gui->DrawSprite(nullptr, Int2(center.x + size / 2 + dist + 1, center.y - size / 2 + 1), Int2(length - 1, size - 1), Color::Black);
	gui->DrawSprite(nullptr, Int2(center.x - size / 2 + 1, center.y - size / 2 - length - dist + 1), Int2(size - 1, length - 1), Color::Black);
	gui->DrawSprite(nullptr, Int2(center.x - size / 2 + 1, center.y + size / 2 + dist + 1), Int2(size - 1, length - 1), Color::Black);
	gui->DrawSprite(nullptr, Int2(center.x - size / 2 - length - dist, center.y - size / 2), Int2(length - 1, size - 1), col);
	gui->DrawSprite(nullptr, Int2(center.x + size / 2 + dist, center.y - size / 2), Int2(length - 1, size - 1), col);
	gui->DrawSprite(nullptr, Int2(center.x - size / 2, center.y - size / 2 - length - dist), Int2(size - 1, length - 1), col);
	gui->DrawSprite(nullptr, Int2(center.x - size / 2, center.y + size / 2 + dist), Int2(size - 1, length - 1), col);
}

void GameGui::AddBubble(cstring text, Unit* unit)
{
	assert(text && unit);
	SpeechBubble* bubble = new SpeechBubble;
	bubble->text = text;
	bubble->unit = unit;
	bubbles.push_back(bubble);
}
