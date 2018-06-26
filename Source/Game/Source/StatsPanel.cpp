#include "GameCore.h"
#include "StatsPanel.h"
#include "Player.h"
#include "Perk.h"
#include <Font.h>
#include <ResourceManager.h>
#include <Gui.h>
#include <Input.h>

StatsPanel::StatsPanel(ResourceManager* res_mgr)
{
	size = Int2(430, 400);

	// label
	Label* label = new Label;
	label->text = "Your stats";
	label->color = Color(0, 255, 33);
	label->font = res_mgr->GetFont("Cestellar", 20);
	label->size = label->CalculateSize();
	label->flags = Font::Center | Font::VCenter;
	label->SetPos(Int2((size.x - label->size.x) / 2, 10));
	Add(label);

	textbox_layout = TextBox::default_layout;
	textbox_layout.flags = Font::Left | Font::Top;
	textbox_layout.color = Color(255, 255, 255, 128);

	// stats
	textbox_stats = new TextBox(textbox_layout);
	textbox_stats->size = Int2(200, 300);
	textbox_stats->SetPos(Int2(10, 50));
	Add(textbox_stats);

	// perks
	textbox_perks = new TextBox(textbox_layout);
	textbox_perks->size = Int2(200, 300);
	textbox_perks->SetPos(Int2(220, 50));
	Add(textbox_perks);

	// button
	Button* bt = new Button;
	bt->text = "OK";
	bt->event = [](int) { gui->CloseDialog(); };
	bt->size = Int2(100, 30);
	bt->SetPos(Int2((size.x - bt->size.x) / 2, size.y - bt->size.y - 10));
	Add(bt);

	visible = false;
}

void StatsPanel::Show(Player* player)
{
	this->player = player;

	UpdateStats();
	UpdatePerks();
	timer = 1.f;
	gui->ShowDialog(this);
}

void StatsPanel::Update(float dt)
{
	Panel::Update(dt);

	if(mouse_focus && gui->GetInput()->PressedOnce(Key::Escape))
		gui->CloseDialog();

	timer -= dt;
	if(timer <= 0.f)
	{
		UpdateStats();
		timer = 1.f;
	}
}

void StatsPanel::UpdateStats()
{
	textbox_stats->text = Format("Hp: %d/%d\nSatiation: %d/%d", player->hp, player->maxhp, player->food, player->maxfood);
}

void StatsPanel::UpdatePerks()
{
	string& s = textbox_perks->text;
	s = "Perks:\n";
	for(std::pair<PerkId, int>& perk : player->perks)
	{
		Perk& info = Perk::perks[(int)perk.first];
		if(perk.second == 1)
			s += Format("%s\n", info.name);
		else
			s += Format("%s (%d)\n", info.name, perk.second);
	}
	if(player->perks.empty())
		s += "(none)";
}
