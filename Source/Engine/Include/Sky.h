#pragma once

struct Sky
{
	Sky();
	void Update(float dt);

	/*void SetTime(float time)
	{
		assert(InRange(time, 0.f, 1.f));
		this->time = time;
	}

	float GetTime() const { return time; }*/

	Vec4 clouds_color[2], // 0-borders, 1-center
		horizon_color,
		zenith_color;
	Vec2 clouds_offset,
		wind_vec;
	float time,
		clouds_scale_inv, // clouds scaling (1-big clouds, 2-medium, 4-small, 8-very small etc)
		clouds_sharpness, // how sharp are clouds edges (0-15)
		clouds_threshold, // how many clouds (0-100%, 0.6-50%, 1-almost none, 1.2-none)
		stars_visibility;
	Texture* tex_clouds_noise;
};
