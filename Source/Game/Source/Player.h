#pragma once

#include "Unit.h"

enum Action
{
	A_NONE,
	A_USE_MEDKIT,
	A_PICKUP,
	A_ATTACK,
	A_EAT,
	A_RELOAD
};

enum FoodLevel
{
	FL_STARVING,
	FL_VERY_HUGRY,
	FL_HUNGRY,
	FL_NORMAL,
	FL_FULL
};

struct Player : Unit
{
	Player();
	void UseMedkit();
	void EatFood();
	void SwitchWeapon(bool melee);
	void Reload();

	FoodLevel GetFoodLevel();

	Action action;
	int action_state, food;
	uint medkits, food_cans, ammo, current_ammo;
	SceneNode* weapon, *hair;
	GroundItem* item_before;
	float rot_buf, last_rot, hungry_timer;
	Item* melee_weapon, *ranged_weapon;
	bool use_melee;

	static const float walk_speed;
	static const float run_speed;
	static const float rot_speed;
	static const float hunger_timestep;
};
