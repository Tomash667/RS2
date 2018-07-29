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
	IDLE_ANIM,
	IDLE_TALK,
	IDLE_TALK_ALONE
};

enum PathfindingState
{
	PF_NOT_USED,
	PF_NOT_GENERATED,
	PF_USED
};

struct Ai : Unit
{
	Ai(UnitType type) : Unit(type), state(AI_IDLE), idle(IDLE_NONE), timer(idle_timer.Random()), attacking(false), pf_timer(0),
		pf_state(PF_NOT_USED), target(nullptr), scan_timer(0) {}
	void ChangeState(AiState new_state);
	void Save(FileWriter& f);
	void Load(FileReader& f);

	AiState state;
	IdleAction idle;
	PathfindingState pf_state;
	vector<Vec3> path;
	Unit* target;
	Vec3 target_pos, start_pos, pf_target;
	float timer, timer2, pf_timer, scan_timer;
	int attack_index, pf_index;
	bool attacking;
	// target, scan_timer NEW FIXME

	static const Vec2 idle_timer;
};
