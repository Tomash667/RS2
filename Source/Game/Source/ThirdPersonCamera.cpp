#include "GameCore.h"
#include "ThirdPersonCamera.h"
#include "Player.h"
#include "Level.h"
#include <Input.h>
#include <Camera.h>
#include <SceneNode.h>

const Vec2 ThirdPersonCamera::c_angle = Vec2(3.68f, 5.75f);
const Vec2 ThirdPersonCamera::c_angle_aim = Vec2(3.24f, 5.47f);

ThirdPersonCamera::ThirdPersonCamera(Camera* cam, Level* level, Input* input) : cam(cam), level(level), input(input)
{
	assert(cam && level && input);

	rot = Vec2(0, 4.47908592f);
	dist = 1.5f;
	shift = 0.25f;
	aim = false;
}

void ThirdPersonCamera::Update(float dt, bool allow_mouse)
{
	Player* player = level->player;

	if(allow_mouse)
	{
		// move camera distance
		/*if(input->Down(Key::Shift))
			cam_shift += 0.25f * input->GetMouseWheel();
		else
			cam_dist -= 0.25f * input->GetMouseWheel();
		if(cam_dist < 0.25f)
			cam_dist = 0.25f;
		else if(cam_dist > 5.f)
			cam_dist = 5.f;
		if(input->Down(Key::MiddleButton))
		{
			cam_dist = 1.5f;
			cam_rot.y = 4.47908592f;
		}*/

		Vec2 angle_limits = player->action == A_AIM ? c_angle_aim : c_angle;
		rot.x = player->node->rot.y;
		rot.y = angle_limits.Clamp(rot.y - float(input->GetMouseDif().y) / 400);
	}

	cam->to = player->node->pos;
	cam->to.y += h;
	Vec3 camera_to_without_shift = cam->to;

	float shift_x = shift * (dist - 0.25f) / 1.25f;
	if(shift_x != 0)
	{
		float angle = -rot.x + PI;
		cam->to += Vec3(sin(angle)*shift_x, 0, cos(angle)*shift_x);
	}

	Vec3 ray(0, -dist, 0);
	Matrix mat = Matrix::Rotation(-rot.x - PI / 2, rot.y, 0);
	ray = Vec3::Transform(ray, mat);

	float t = min(level->RayTest(cam->to, ray), level->RayTest(camera_to_without_shift, ray));

	shift_x *= t;
	if(shift_x != 0)
	{
		float angle = -rot.x + PI;
		cam->to = camera_to_without_shift + Vec3(sin(angle)*shift_x, 0, cos(angle)*shift_x);
	}
	cam->from = cam->to + ray * t;

	if(ray.Length() * t < 0.3f)
	{
		player->node->visible = false;
		player->hair->visible = false;
	}
	else
	{
		player->node->visible = true;
		player->hair->visible = true;
	}
}

void ThirdPersonCamera::SetAim(bool enabled)
{
	if(aim == enabled)
		return;
	aim = enabled;
	if(enabled)
	{
		dist = 0.5f;
		shift = 1.f;
	}
	else
	{
		dist = 1.5f;
		shift = 0.25f;
	}
}
