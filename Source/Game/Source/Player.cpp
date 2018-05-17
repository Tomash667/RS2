#include "GameCore.h"
#include "Player.h"
#include "Item.h"
#include <SceneNode.h>
#include <MeshInstance.h>

Player::Player() : Unit(false), medkits(0), action(A_NONE), item_before(nullptr), rot_buf(0), last_rot(0)
{
	melee_weapon = Item::Get("baseball_bat");
}

void Player::UseMedkit()
{
	if(action == A_NONE && hp != 100 && medkits != 0)
	{
		action = A_USE_MEDKIT;
		node->mesh_inst->Play("use", PLAY_ONCE | PLAY_CLEAR_FRAME_END_INFO, 1);
	}
}
