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


void GameGui::Init(Engine* engine, Player* player)
{
	this->engine = engine;
	this->player = player;

	const Int2& wnd_size = gui->GetWindowSize();
	ResourceManager* res_mgr = engine->GetResourceManager();

	gui->SetCursorTexture(res_mgr->GetTexture("cursor.png"));

	// crosshair
	sprite_crosshair = new Sprite;
	sprite_crosshair->image = res_mgr->GetTexture("crosshair_dot.png");
	sprite_crosshair->size = Int2(16, 16);
	sprite_crosshair->pos = (wnd_size - sprite_crosshair->size) / 2;
	Add(sprite_crosshair);

	// hp bar
	hp_bar = new ProgressBar;
	hp_bar->image = res_mgr->GetTexture("hp_bar.png");
	hp_bar->background = res_mgr->GetTexture("bar_empty.png");
	hp_bar->size = Int2(256, 24);
	hp_bar->pos.y = wnd_size.y - hp_bar->size.y;
	Add(hp_bar);

	// medkit icon & counter
	Sprite* sprite = new Sprite;
	sprite->image = res_mgr->GetTexture("medkit_icon.png");
	sprite->size = Int2(32, 32);
	sprite->pos = Int2(256 + 4, wnd_size.y - 32);
	Add(sprite);

	label_medkits = new Label;
	label_medkits->pos = sprite->pos + Int2(2, 10);
	label_medkits->size = Int2(100, 100);
	Add(label_medkits);

	// food icon
	sprite_food = new Sprite;
	sprite_food->image = res_mgr->GetTexture("food_icon.png");
	sprite_food->size = Int2(32, 32);
	sprite_food->pos = Int2(8, hp_bar->pos.y - 40);
	sprite_food->visible = false;
	Add(sprite_food);

	// ammo counter
	label_ammo = new Label;
	label_ammo->pos = Int2(sprite->pos + Int2(8, 0));
	label_ammo->size = Int2(100, 32);
	label_ammo->visible = false;
	Add(label_ammo);

	// fps panel
	label_fps = new Label;
	label_fps->pos = Int2(6, 6);
	label_fps->color = Color(0, 255, 33, 255);
	panel_fps = new Panel;
	panel_fps->image = res_mgr->GetTexture("panel.png");
	panel_fps->corners = Int2(6, 32);
	panel_fps->color = Color(255, 255, 255, 200);
	panel_fps->visible = false;
	panel_fps->Add(label_fps);
	Add(panel_fps);

	// inventory
	inventory = new Inventory(res_mgr, player);
	inventory->pos = Int2(wnd_size.x - inventory->size.x, wnd_size.y - inventory->size.y);
	Add(inventory);

	engine->GetGui()->Add(this);
}

void GameGui::Draw()
{
	Container::Draw();

	GroundItem* item = player->item_before;
	if(item)
	{
		Vec3 item_pos = item->node->pos;
		item_pos.y += 0.5f;

		Int2 text_pos;
		if(!gui->To2dPoint(item_pos, text_pos))
			return;

		cstring text = Format("[E] %s", item->item->name);

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
}

void GameGui::Update()
{
	Input* input = gui->GetInput();

	// fps panel
	if(input->Pressed(Key::F1))
		panel_fps->visible = !panel_fps->visible;
	if(panel_fps->visible)
	{
		label_fps->text = Format("Fps: %g\nPos: %g; %g; %g\nRot: %g %s", FLT10(engine->GetFps()),
			FLT10(player->node->pos.x), FLT10(player->node->pos.y), FLT10(player->node->pos.z),
			FLT10(player->node->rot.y), dir_name[AngleToDir(player->node->rot.y)]);
		label_fps->size = label_fps->CalculateSize();
		Int2 panel_size = label_fps->size + Int2(2 * panel_fps->corners.x, 2 * panel_fps->corners.x);
		if(panel_size > panel_fps->size)
			panel_fps->size = Int2::Max(panel_size, panel_fps->size);
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

	// show/hide inventory
	if(player->hp > 0)
	{
		if(input->Pressed(Key::I))
			inventory->Show(!inventory->visible);
	}
	else if(inventory->visible)
		inventory->Show(false);

	sprite_crosshair->visible = !inventory->visible;

	if(inventory->visible)
		inventory->Update();
}

bool GameGui::IsInventoryOpen()
{
	return inventory->visible;
}
