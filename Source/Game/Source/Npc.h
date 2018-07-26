#pragma once

#include "Ai.h"

struct Npc : Ai
{
	Npc() : Ai(UNIT_NPC), attack_player(false) {}
	void Save(FileWriter& f) override;
	void Load(FileReader& f) override;

	SceneNode* weapon;
	bool attack_player;
};