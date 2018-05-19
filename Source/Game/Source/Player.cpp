#include "GameCore.h"
#include "Player.h"
#include "Item.h"
#include <SceneNode.h>
#include <MeshInstance.h>


const float Player::walk_speed = 2.5f;
const float Player::run_speed = 7.f;
const float Player::rot_speed = 4.f;
const float Player::hunger_timestep = 10.f;


Player::Player() : Unit(false), medkits(0), food_cans(0), action(A_NONE), item_before(nullptr), rot_buf(0), last_rot(0), food(80),
hungry_timer(hunger_timestep)
{
	melee_weapon = Item::Get("baseball_bat");
}

void Player::UseMedkit()
{
	if(action == A_NONE && hp != 100 && medkits != 0)
	{
		action = A_USE_MEDKIT;
		action_state = 0;
		node->mesh_inst->Play("use", PLAY_ONCE | PLAY_CLEAR_FRAME_END_INFO, 1);
		weapon->visible = false;
	}
}

void Player::EatFood()
{
	if(action == A_NONE && food_cans != 0 && GetFoodLevel() != FL_FULL)
	{
		action = A_EAT;
		action_state = 0;
		node->mesh_inst->Play("je", PLAY_ONCE | PLAY_CLEAR_FRAME_END_INFO, 1);
		weapon->visible = false;
	}
}

FoodLevel Player::GetFoodLevel()
{
	if(food >= 90)
		return FL_FULL;
	else if(food >= 33)
		return FL_NORMAL;
	else if(food >= 15)
		return FL_HUNGRY;
	else if(food >= 0)
		return FL_VERY_HUGRY;
	else
		return FL_STARVING;
}

void Player::Save(FileWriter& f)
{

}

void Player::Load(FileReader& f)
{

}
