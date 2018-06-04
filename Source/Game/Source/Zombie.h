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
	Zombie() : Unit(true), state(AI_IDLE), idle(IDLE_NONE), timer(idle_timer.Random()), attacking(false), pf_used(false), pf_timer(0) {}
	void ChangeState(AiState new_state);

	AiState state;
	IdleAction idle;
	Vec3 target_pos, start_pos;
	float timer, timer2, pf_timer;
	vector<Int2> path;
	Int2 pf_target;
	int attack_index;
	bool attacking, pf_used;

	static const float walk_speed;
	static const float rot_speed;
	static const Vec2 idle_timer;
};
