#include "GameCore.h"
#include "Zombie.h"

const float Zombie::walk_speed = 1.5f;
const float Zombie::rot_speed = 2.5f;
const Vec2 Zombie::idle_timer = Vec2(4.f, 6.f);

void Zombie::ChangeState(AiState new_state)
{
	if(new_state == AI_COMBAT)
	{
		timer = 0.f; // next attack
		timer2 = 0.f; // lost target out of sight
	}
	else if(new_state == AI_IDLE)
	{
		idle = IDLE_NONE;
		timer = idle_timer.Random(); // next idle action
	}
	state = new_state;
}

void Zombie::Save(FileWriter& f)
{
	Unit::Save(f);
	if(IsAlive())
	{
		f << state;
		f << idle;
		f << target_pos;
		f << start_pos;
		f << timer;
		f << timer2;
		f << pf_timer;
		f << pf_state;
		if(pf_state == PF_USED)
		{
			f << pf_target;
			f << pf_index;
			f << path;
		}

		f << attacking;
		if(attacking)
			f << attack_index;
	}
	else
		f << death_timer;
}

void Zombie::Load(FileReader& f)
{
	Unit::Load(f);

	if(IsAlive())
	{
		f >> state;
		f >> idle;
		f >> target_pos;
		f >> start_pos;
		f >> timer;
		f >> timer2;
		f >> pf_timer;
		f >> pf_state;
		if(pf_state == PF_USED)
		{
			f >> pf_target;
			f >> pf_index;
			f >> path;
		}

		f >> attacking;
		if(attacking)
			f >> attack_index;
	}
	else
		f >> death_timer;
}
