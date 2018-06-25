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

	Vec4 sky_color;
	Vec2 clouds_offset, wind_vec;

//private:
	float time;
	float clouds_scale_inv; // TODO check what this do
	float clouds_sharpness; // ???
	float clouds_threshold; // ???
	Vec4 clouds_color[2];
	Texture* tex_clouds_noise;
};
