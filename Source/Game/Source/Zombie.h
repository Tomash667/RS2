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

enum PathfindingState
{
	PF_NOT_USED,
	PF_NOT_GENERATED,
	PF_USED
};

struct Zombie : Unit
{
	Zombie() : Unit(true), state(AI_IDLE), idle(IDLE_NONE), timer(idle_timer.Random()), attacking(false), pf_timer(0), pf_state(PF_NOT_USED) {}
	void ChangeState(AiState new_state);
	void Save(FileWriter& f);
	void Load(FileReader& f);

	AiState state;
	IdleAction idle;
	PathfindingState pf_state;
	vector<Vec3> path;
	Vec3 target_pos, start_pos, pf_target;
	float timer, timer2, pf_timer;
	int attack_index, death_timer, pf_index;
	bool attacking;

	static const float walk_speed;
	static const float rot_speed;
	static const Vec2 idle_timer;
};
