#include "GameCore.h"
#include "PickPerkDialog.h"
#include "Player.h"
#include "Perk.h"
#include <ResourceManager.h>
#include <Font.h>
#include <Gui.h>
#include "GameState.h"

PickPerkDialog::PickPerkDialog(GameState* game_state, ResourceManager* res_mgr)
{
	this->game_state = game_state;
	size = Int2(600, 400);

	// label
	Label* label = new Label;
	label->text = "You survived another night, pick a new perk";
	label->color = Color(0, 255, 33);
	label->font = res_mgr->GetFont("Cestellar", 20);
	label->size = label->CalculateSize();
	label->flags = Font::Center | Font::VCenter;
	label->SetPos(Int2((size.x - label->size.x) / 2, 10));
	Add(label);

	// perks
	list_perks = new ListBox;
	list_perks->size = Int2(300, 150);
	list_perks->SetPos(Int2((size.x - list_perks->size.x) / 2, 60));
	list_perks->Init();
	Add(list_perks);

	// perk desc
	textbox_desc = new TextBox;
	textbox_desc->size = Int2(500, 70);
	textbox_desc->SetPos(Int2((size.x - textbox_desc->size.x) / 2, 220));
	Add(textbox_desc);

	// button
	bt = new Button;
	bt->text = "OK";
	bt->event = delegate<void(int)>(this, &PickPerkDialog::OnEvent);
	bt->size = Int2(100, 30);
	bt->SetPos(Int2((size.x - bt->size.x) / 2, size.y - bt->size.y - 30));
	Add(bt);

	visible = false;
}

void PickPerkDialog::Update(float dt)
{
	Panel::Update(dt);

	if(list_perks->selected_index != last_index)
	{
		last_index = list_perks->selected_index;
		textbox_desc->text = Perk::perks[(int)perks[last_index].first].desc;
		bt->state = Button::UP;
	}
}

void PickPerkDialog::Show(Player* player)
{
	this->player = player;
	player->GetAvailablePerks(perks);
	if(perks.empty())
		return;

	TakeRandomElements(perks, 5u);

	list_perks->items.resize(perks.size());
	for(uint i = 0; i < perks.size(); ++i)
	{
		std::pair<PerkId, int>& perk = perks[i];
		Perk& perk_info = Perk::perks[(int)perk.first];
		ListBox::Item& item = list_perks->items[i];
		item.value = i;
		if(perk.second == 1)
			item.text = perk_info.name;
		else
			item.text = Format("%s (%d)", perk_info.name, perk.second);
	}
	list_perks->selected_index = -1;
	last_index = -1;

	textbox_desc->text.clear();
	bt->state = Button::DISABLED;

	game_state->SetPaused(true);
	gui->ShowDialog(this);
}

void PickPerkDialog::OnEvent(int id)
{
	PerkId perk = perks[last_index].first;
	player->AddPerk(perk);
	game_state->SetPaused(false);
	gui->CloseDialog();
}
