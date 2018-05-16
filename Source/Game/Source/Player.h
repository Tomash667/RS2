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
	Player() : Unit(false), medkits(0), action(A_NONE), item_before(nullptr), rot_buf(0), last_rot(0) {}

	Action action;
	int medkits, action_state;
	SceneNode* weapon, *hair;
	GroundItem* item_before;
	float rot_buf, last_rot;
	static const float walk_speed;
	static const float run_speed;
	static const float rot_speed;
};
