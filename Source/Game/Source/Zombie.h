#pragma once

#include "Unit.h"

enum AiState
{
	AI_IDLE,
	AI_COMBAT,
	AI_FALLOW
};

enum IdleAction
{
	IDLE_NONE,
	IDLE_ROTATE,
	IDLE_WALK,
	IDLE_ANIM
};

struct Zombie : Unit
{
	Zombie() : Unit(true), state(AI_IDLE), idle(IDLE_NONE), timer(idle_timer.Random()), attacking(false) {}

	AiState state;
	IdleAction idle;
	Vec3 target_pos;
	float timer;
	int attack_index;
	bool attacking;

	static const float walk_speed;
	static const float rot_speed;
	static const Vec2 idle_timer;
};
