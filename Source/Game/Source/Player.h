#pragma once

#include "Unit.h"

enum Action
{
	A_NONE,
	A_USE_MEDKIT,
	A_PICKUP,
	A_ATTACK,
	A_EAT,
	A_RELOAD,
	A_AIM
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
	Player(Level* level);
	void UseMedkit();
	void EatFood();
	void SwitchWeapon(bool melee);
	void Reload();
	void Save(FileWriter& f);
	void Load(FileReader& f);
	void AddPerk(PerkId id);

	FoodLevel GetFoodLevel();
	Vec3 GetShootPos();
	void GetAvailablePerks(vector<std::pair<PerkId, int>>& available_perks);
	int GetPerkLevel(PerkId id);
	float GetRunSpeed();

	Level* level;
	Action action;
	int action_state, food, maxfood;
	uint medkits, food_cans, ammo, current_ammo;
	SceneNode* weapon, *hair;
	GroundItem* item_before;
	float rot_buf, last_rot, hungry_timer, shot_delay, idle_timer, idle_timer_max, aim;
	Item* melee_weapon, *ranged_weapon;
	vector<std::pair<PerkId, int>> perks;
	bool use_melee, death_starved;

	static const float walk_speed;
	static const float run_speed;
	static const float rot_speed;
	static const float hunger_timestep;
};
