#pragma once

struct ThirdPersonCamera
{
	ThirdPersonCamera(Camera* cam, Level* level, Input* input);
	void Update(float dt, bool allow_mouse);
	void SetAim(bool enabled);
	void Save(FileWriter& f);
	void Load(FileReader& f);

	Camera* cam;
	Level* level;
	Input* input;
	Vec2 rot;
	float dist, h, shift, springiness;
	bool aim, reset;

	static const Vec2 c_angle;
	static const Vec2 c_angle_aim;
};
