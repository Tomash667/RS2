#include "GameCore.h"
#include "Player.h"
#include "Item.h"
#include <SceneNode.h>
#include <MeshInstance.h>
#include "Level.h"
#include <Scene.h>
#include "Perk.h"

const float Player::hunger_timestep = 10.f;

Player::Player(Level* level) : Unit(UNIT_PLAYER), level(level), medkits(0), food_cans(0), action(A_NONE), item_before(nullptr), rot_buf(0),
last_rot(0), food(80), maxfood(100), hungry_timer(hunger_timestep), ranged_weapon(nullptr), ammo(0), current_ammo(0), use_melee(true),
idle_timer_max(Random(2.5f, 4.f)), aim(0), last_survived_day(0), unit_before(nullptr)
{
	melee_weapon = Item::Get("baseball_bat");
	idle_timer = idle_timer_max;
}

void Player::UseMedkit()
{
	if(action == A_NONE && hp != maxhp && medkits != 0)
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
	float ratio = float(food) / maxfood;
	if(ratio >= 0.9f)
		return FL_FULL;
	else if(ratio >= 0.33f)
		return FL_NORMAL;
	else if(ratio >= 0.15f)
		return FL_HUNGRY;
	else if(ratio >= 0)
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
		f << maxfood;
		f << hungry_timer;
		f << last_survived_day;
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

	f << perks;
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
		f >> maxfood;
		f >> hungry_timer;
		f >> last_survived_day;
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

	f >> perks;

	item_before = nullptr;
	unit_before = nullptr;
	if(!use_melee)
	{
		weapon->mesh = ranged_weapon->mesh;
		weapon->SetParentPoint(node->mesh->GetPoint("pistol"));
		level->scene->RecycleMeshInstance(weapon);
	}
}

void Player::GetAvailablePerks(vector<std::pair<PerkId, int>>& available_perks)
{
	available_perks.clear();
	for(int i = 0; i < (int)PerkId::Max; ++i)
	{
		bool found = false;
		for(std::pair<PerkId, int>& perk : perks)
		{
			if(perk.first == (PerkId)i)
			{
				if(perk.second != Perk::max_level)
					available_perks.push_back({ perk.first, perk.second + 1 });
				found = true;
				break;
			}
		}
		if(!found)
			available_perks.push_back({ (PerkId)i, 1 });
	}
}

void Player::AddPerk(PerkId id)
{
	if(id == PerkId::Tough)
	{
		float ratio = GetHpp();
		maxhp += 20;
		hp = int(ratio * maxhp);
	}
	else if(id == PerkId::LightEater)
	{
		float ratio = float(food) / maxfood;
		maxfood += 20;
		food = int(ratio * maxfood);
	}

	for(std::pair<PerkId, int>& perk : perks)
	{
		if(perk.first == id)
		{
			++perk.second;
			return;
		}
	}
	perks.push_back({ id, 1 });
}

int Player::GetPerkLevel(PerkId id)
{
	for(std::pair<PerkId, int>& perk : perks)
	{
		if(perk.first == id)
			return perk.second;
	}
	return 0;
}

float Player::GetRunSpeed()
{
	int level = GetPerkLevel(PerkId::Agile);
	return GetStats().run_speed + 0.5f * level;
}

void Player::UpdateAim(float mod)
{
	int level = GetPerkLevel(PerkId::Firearms);
	mod *= 1.f - 0.2f * level;
	aim += mod;
}
