#pragma once

#include "Unit.h"

enum Action
{
	A_NONE,
	A_USE_MEDKIT,
	A_PICKUP,
	A_ATTACK,
	A_EAT
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
	void Save(FileWriter& f);
	void Load(FileReader& f);

	FoodLevel GetFoodLevel();

	Action action;
	int action_state, food;
	uint medkits, food_cans;
	SceneNode* weapon, *hair;
	GroundItem* item_before;
	float rot_buf, last_rot, hungry_timer;
	Item* melee_weapon;
	bool death_starved;

	static const float walk_speed;
	static const float run_speed;
	static const float rot_speed;
	static const float hunger_timestep;
};
