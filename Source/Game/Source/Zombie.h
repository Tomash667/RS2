#pragma once

#include "Ai.h"

struct Zombie : Ai
{
	Zombie() : Ai(UNIT_ZOMBIE) {}

	static const float walk_speed;
	static const float rot_speed;
};
