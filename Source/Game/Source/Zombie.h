#pragma once

#include "Unit.h"

struct Zombie : Unit
{
	Zombie() : Unit(true), next_attack(0), attacking(false), active(false) {}

	float next_attack;
	int attack_index;
	bool attacking, active;
	static const float walk_speed;
	static const float rot_speed;
};
