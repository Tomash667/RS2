#include "GameCore.h"
#include "Unit.h"
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
