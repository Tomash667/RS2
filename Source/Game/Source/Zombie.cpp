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
