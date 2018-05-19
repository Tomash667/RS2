#include "GameCore.h"
#include "Inventory.h"
#include "Player.h"
#include "Item.h"
#include <ResourceManager.h>
#include <Gui.h>
#include <Font.h>
#include <Input.h>

const uint Inventory::grid_size = 64;

Inventory::Inventory(ResourceManager* res_mgr, Player* player)
{
	this->player = player;
	tooltip_index = -1;

	image = res_mgr->GetTexture("panel.png");
	size = Int2(grid_size * SLOT_MAX + 16, grid_size + 40);
	corners = Int2(6, 32);
	color = Color(255, 255, 255, 200);
	visible = false;

	tex_grid = res_mgr->GetTexture("grid.png");
}

void Inventory::Show(bool show)
{
	gui->SetCursorVisible(show);
	visible = show;
	tooltip_index = -1;
}

void Inventory::Draw()
{
	Panel::Draw();

	gui->DrawText("Inventory", nullptr, Color(0, 255, 33), Font::Center | Font::VCenter, Rect(pos.x, pos.y + 4, pos.x + size.x, pos.y + 24));

	Int2 offset = pos + Int2(8, 30);
	Int2 s(grid_size, grid_size);

	for(int i = 0; i < SLOT_MAX; ++i)
	{
		Slot& slot = slots[i];
		gui->DrawSprite(tex_grid, offset, s);
		if(slot.item)
		{
			gui->DrawSprite(slot.item->icon, offset, s);
			if(slot.count != 0)
				gui->DrawText(Format("%u", slot.count), nullptr, Color(0, 255, 33), Font::Bottom, Rect::Create(offset + Int2(4, 0), s));
		}
		offset.x += grid_size;
	}

	if(tooltip_index != -1)
	{
		cstring text = GetTooltipText();
		Int2 text_size = gui->GetDefaultFont()->CalculateSize(text);
		Int2 text_pos = gui->GetCursorPos() + Int2(0, 32);
		const Int2& wnd_size = gui->GetWindowSize();

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

void Inventory::Update()
{
	PrepareSlots();
	tooltip_index = -1;

	Input* input = gui->GetInput();
	Int2 offset = pos + Int2(8, 30);
	Int2 s(grid_size, grid_size);
	const Int2& cursor_pos = gui->GetCursorPos();

	if(input->Pressed(Key::Escape)
		|| (!Rect::IsInside(pos, size, cursor_pos) && input->PressedOnce(Key::LeftButton)))
	{
		Show(false);
		return;
	}

	for(int i = 0; i < SLOT_MAX; ++i)
	{
		if(Rect::IsInside(offset, s, cursor_pos))
		{
			Slot& slot = slots[i];
			if(!slot.item)
				break;
			tooltip_index = i;
			if(gui->GetInput()->Pressed(Key::LeftButton))
				UseItem((SLOT)i);
			break;
		}
		offset.x += grid_size;
	}
}

void Inventory::PrepareSlots()
{
	slots[SLOT_MELEE_WEAPON] = Slot(player->melee_weapon);
	slots[SLOT_RANGED_WEAPON] = Slot(nullptr);
	slots[SLOT_AMMO] = Slot(nullptr);
	slots[SLOT_FOOD] = (player->food_cans > 0 ? Slot(Item::Get("canned_food"), player->food_cans) : Slot(nullptr));
	slots[SLOT_MEDKIT] = (player->medkits > 0 ? Slot(Item::Get("medkit"), player->medkits) : Slot(nullptr));
}

cstring Inventory::GetTooltipText()
{
	Slot& slot = slots[tooltip_index];
	if(slot.count != 0u)
		return Format("%s (%u)", slot.item->name, slot.count);
	else
		return slot.item->name;
}

void Inventory::UseItem(SLOT slot)
{
	switch(slot)
	{
	case SLOT_MEDKIT:
		player->UseMedkit();
		break;
	case SLOT_FOOD:
		player->EatFood();
		break;
	}
}
