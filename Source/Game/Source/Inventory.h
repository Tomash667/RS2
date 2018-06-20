#pragma once

#include "GuiControls.h"

class Inventory : public CustomPanel
{
public:
	Inventory(ResourceManager* res_mgr, GameState* game_state);
	void Show(bool show);
	void Draw() override;
	void Update(float dt) override;

private:
	struct Slot
	{
		Slot() {}
		Slot(Item* item, uint count = 0) : item(item), count(count) {}

		Item* item;
		uint count;
	};

	enum SLOT
	{
		SLOT_MELEE_WEAPON,
		SLOT_RANGED_WEAPON,
		SLOT_AMMO,
		SLOT_FOOD,
		SLOT_MEDKIT,
		SLOT_MAX
	};

	void PrepareSlots();
	cstring GetTooltipText();
	void UseItem(SLOT slot);

	GameState* game_state;
	Slot slots[SLOT_MAX];
	Texture* tex_grid;
	int tooltip_index;

	static const uint grid_size;
};
