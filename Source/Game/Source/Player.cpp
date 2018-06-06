#include "GameCore.h"
#include "Player.h"
#include "Item.h"
#include <SceneNode.h>
#include <MeshInstance.h>
#include "Level.h"
#include <Scene.h>

const float Player::walk_speed = 2.5f;
const float Player::run_speed = 7.f;
const float Player::rot_speed = 4.f;
const float Player::hunger_timestep = 10.f;

Player::Player(Level* level) : Unit(false), level(level), medkits(0), food_cans(0), action(A_NONE), item_before(nullptr), rot_buf(0), last_rot(0), food(80),
hungry_timer(hunger_timestep), ranged_weapon(nullptr), ammo(0), current_ammo(0), use_melee(true), idle_timer_max(Random(2.5f, 4.f)), aim(0)
{
	melee_weapon = Item::Get("baseball_bat");
	idle_timer = idle_timer_max;
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

void Player::SwitchWeapon(bool melee)
{
	bool have_weapon = (melee ? melee_weapon : ranged_weapon) != nullptr;
	if(action == A_NONE && use_melee != melee && have_weapon)
	{
		use_melee = melee;
		weapon->mesh = melee ? melee_weapon->mesh : ranged_weapon->mesh;
		weapon->SetParentPoint(node->mesh->GetPoint(melee ? "bron" : "pistol"));
		level->scene->RecycleMeshInstance(weapon);
	}
}

void Player::Reload()
{
	if(!use_melee && current_ammo != 10 && ammo > 0 && (action == A_NONE || (action == A_AIM && shot_delay <= 0.f)))
	{
		action = A_RELOAD;
		action_state = 0;
		node->mesh_inst->Play("use", PLAY_ONCE | PLAY_CLEAR_FRAME_END_INFO, 1);
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

Vec3 Player::GetShootPos()
{
	Mesh::Point* hitbox = weapon->mesh->FindPoint("hit");
	Mesh::Point* bone = (Mesh::Point*)weapon->GetParentPoint();
	node->mesh_inst->SetupBones();
	Matrix m = hitbox->mat
		* bone->mat
		* node->mesh_inst->GetMatrixBones()[bone->bone]
		* Matrix::RotationY(-node->rot.y)
		* Matrix::Translation(node->pos);
	return Vec3::TransformZero(m);
}

void Player::Save(FileWriter& f)
{
	Unit::Save(f);

	if(IsAlive())
	{
		f << action;
		f << action_state;
		f << idle_timer;
		f << idle_timer_max;
		f << rot_buf;
		f << last_rot;
		f << shot_delay;
		f << aim;
		f << food;
		f << hungry_timer;
	}
	else
	{
		f << dying;
		f << death_starved;
	}

	// inventory
	f << (melee_weapon ? melee_weapon->id : "");
	f << (ranged_weapon ? ranged_weapon->id : "");
	f << medkits;
	f << food_cans;
	f << ammo;
	f << current_ammo;
	f << use_melee;
}

void Player::Load(FileReader& f)
{
	Unit::Load(f);

	if(IsAlive())
	{
		f >> action;
		f >> action_state;
		f >> idle_timer;
		f >> idle_timer_max;
		f >> rot_buf;
		f >> last_rot;
		f >> shot_delay;
		f >> aim;
		f >> food;
		f >> hungry_timer;
	}
	else
	{
		f >> dying;
		f >> death_starved;
	}

	// inventory
	melee_weapon = Item::Get(f.ReadString1());
	ranged_weapon = Item::Get(f.ReadString1());
	f >> medkits;
	f >> food_cans;
	f >> ammo;
	f >> current_ammo;
	f >> use_melee;

	item_before = nullptr;
	if(!use_melee)
	{
		weapon->mesh = ranged_weapon->mesh;
		weapon->SetParentPoint(node->mesh->GetPoint("pistol"));
		level->scene->RecycleMeshInstance(weapon);
	}
}
