#pragma once

#include "Unit.h"

struct Npc : Unit
{
	Npc() : Unit(UNIT_NPC), attack_player(false) {}
	void Save(FileWriter& f) override;
	void Load(FileReader& f) override;

	bool attack_player;
};
