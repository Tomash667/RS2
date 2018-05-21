#pragma once

struct ThirdPersonCamera
{
	ThirdPersonCamera(Camera* cam, Level* level, Input* input);
	void Update(float dt, bool allow_mouse);
	void SetAim(bool enabled);

	Camera* cam;
	Level* level;
	Input* input;
	Vec2 rot;
	float dist, h, shift;
	bool aim;

	static const Vec2 c_angle;
	static const Vec2 c_angle_aim;
};
