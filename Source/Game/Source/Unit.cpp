#include "GameCore.h"
#include "Unit.h"
#include "Ai.h"
#include "SceneNode.h"
#include "MeshInstance.h"

const float Unit::radius = 0.3f;
const float Unit::height = 1.73f;

void Unit::Update(Animation new_animation)
{
	if(new_animation == animation)
		return;

	switch(new_animation)
	{
	case ANI_STAND:
		node->mesh_inst->Play("stoi", 0, 0);
		break;
	case ANI_ROTATE_LEFT:
		node->mesh_inst->Play("w_lewo", 0, 0);
		break;
	case ANI_ROTATE_RIGHT:
		node->mesh_inst->Play("w_prawo", 0, 0);
		break;
	case ANI_WALK:
		node->mesh_inst->Play("idzie", 0, 0);
		break;
	case ANI_WALK_BACK:
		node->mesh_inst->Play("idzie", PLAY_BACK, 0);
		break;
	case ANI_RUN:
		node->mesh_inst->Play("biegnie", 0, 0);
		break;
	case ANI_IDLE:
		node->mesh_inst->Play(type == UNIT_ZOMBIE ? "idle" : "drapie", PLAY_ONCE | PLAY_CLEAR_FRAME_END_INFO, 0);
		break;
	}
	animation = new_animation;
}

Box Unit::GetBox() const
{
	Box box;
	box.v1.x = node->pos.x - radius;
	box.v1.y = node->pos.y;
	box.v1.z = node->pos.z - radius;
	box.v2.x = node->pos.x + radius;
	box.v2.y = node->pos.y + height;
	box.v2.z = node->pos.z + radius;

	return box;
}

Vec3 Unit::GetSoundPos() const
{
	Vec3 pos = node->pos;
	pos.y += 1.7f;
	return pos;
}

float Unit::GetAngleDiff(const Vec3& target) const
{
	float required_rot = Vec3::Angle2d(node->pos, target);
	float dif = AngleDiff(node->rot.y, required_rot);
	return dif;
}

void Unit::Save(FileWriter& f)
{
	node->mesh_inst->Save(f);
	f << node->pos;
	f << node->rot.y;

	f << hp;
	f << maxhp;
	if(!IsAlive())
	{
		f << dying;
		f << death_timer;
	}
	f << last_damage;
	f << animation;
}

void Unit::Load(FileReader& f)
{
	node->mesh_inst->Load(f);
	f >> node->pos;
	f >> node->rot.y;
	node->rot.x = 0;
	node->rot.z = 0;

	f >> hp;
	f >> maxhp;
	if(!IsAlive())
	{
		f >> dying;
		f >> death_timer;
	}
	f >> last_damage;
	f >> animation;
}
