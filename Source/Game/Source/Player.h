#pragma once

#include "Unit.h"

enum Action
{
	A_NONE,
	A_USE_MEDKIT,
	A_PICKUP,
	A_ATTACK
};

struct Player : Unit
{
	Player();
	void UseMedkit();
	void Save(FileWriter& f);
	void Load(FileReader& f);

	Action action;
	int action_state;
	uint medkits;
	SceneNode* weapon, *hair;
	GroundItem* item_before;
	float rot_buf, last_rot;
	Item* melee_weapon;

	static const float walk_speed;
	static const float run_speed;
	static const float rot_speed;
};
