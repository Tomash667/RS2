#pragma once

#include "Ai.h"

enum TalkMode
{
	TALK_OTHER,
	TALK_ALONE,
	TALK_ESCAPE
};

struct Npc : Ai
{
	Npc() : Ai(UNIT_NPC), attack_player(false) {}
	void Save(FileWriter& f) override;
	void Load(FileReader& f) override;
	void Talk(TalkMode mode);
	bool CanTalk();

	SceneNode* weapon;
	bool attack_player;
};
