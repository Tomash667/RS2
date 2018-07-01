#include "EngineCore.h"
#include "Sky.h"

const float Sky::SKYDOME_RADIUS = 10.f;

Sky::Sky() : time(0), time_period(0), clouds_offset(0, 0), wind_vec(-0.25, 0.12f), clouds_scale_inv(1.f), tex_clouds_noise(nullptr), clouds_sharpness(5.f),
clouds_threshold(0.7f), clouds_color{ Vec4(1,1,1,1), Vec4(1,1,1,1) }, stars_visibility(0.f), tex_sun_glow(nullptr)
{
	// default sky colors
	// 0-5 dark, 5-8 lighting, 8-17 light, 17-19 darkening, 19-24 dark
	sky_colors.Add(0.f / 24, ColorPair(Vec4(8.f / 255, 8.f / 255, 22.f / 255, 1.f), Vec4(8.f / 255, 8.f / 255, 22.f / 255, 1.f)));
	sky_colors.Add(5.f / 24, ColorPair(Vec4(8.f / 255, 8.f / 255, 22.f / 255, 1.f), Vec4(8.f / 255, 8.f / 255, 22.f / 255, 1.f)));
	sky_colors.Add(8.f / 24, ColorPair(Vec4(116.f / 255, 138.f / 255, 168.f / 255, 1.f), Vec4(47.f / 255, 77.f / 255, 128.f / 255, 1.f)));
	sky_colors.Add(17.f / 24, ColorPair(Vec4(116.f / 255, 138.f / 255, 168.f / 255, 1.f), Vec4(47.f / 255, 77.f / 255, 128.f / 255, 1.f)));
	sky_colors.Add(19.f / 24, ColorPair(Vec4(8.f / 255, 8.f / 255, 22.f / 255, 1.f), Vec4(8.f / 255, 8.f / 255, 22.f / 255, 1.f)));
	sky_colors.Add(24.f / 24, ColorPair(Vec4(8.f / 255, 8.f / 255, 22.f / 255, 1.f), Vec4(8.f / 255, 8.f / 255, 22.f / 255, 1.f)));

	// sun settings
	sun.enabled = false;
	sun.size = 0.2f;
	sun.period = 1.f;
	sun.phase = -PI / 2;
	sun.orbit_normal = Vec3(0, 0, 1);
	sun.color = Vec4(1, 1, 1, 1);

	// moon settings
	moon.enabled = false;
	moon.size = 0.05f;
	moon.period = 2.f;
	moon.phase = 0;
	moon.orbit_normal = Vec3(0, 1, 0);
	moon.color = Vec4(1, 1, 1, 1);
}

void Sky::Update(float dt)
{
	clouds_offset += wind_vec * dt;
	time += dt / 10;
	if(time >= 1.f)
		time -= 1.f;
	time_period += dt / 10;
	if(time_period >= 4.f)
		time_period -= 4.f;

	ColorPair colors = sky_colors.Get(time);
	horizon_color = colors[0];
	zenith_color = colors[1];

	if(time < 5.f / 24 || time >= 19.f / 24)
	{
		stars_visibility = 1.f;
		clouds_color[0] = Vec4(0.1f, 0.1f, 0.1f, 1);
	}
	else if(time < 6.f / 24)
	{
		float t = 1.f - (time - 5.f / 24) * 24;
		stars_visibility = t;
		clouds_color[0] = Vec4::Lerp(Vec4(1, 1, 1, 1), Vec4(0.1f, 0.1f, 0.1f, 1), t);
	}
	else if(time > 18.f / 24)
	{
		float t = (time - 18.f / 24) * 24;
		stars_visibility = t;
		clouds_color[0] = Vec4::Lerp(Vec4(1, 1, 1, 1), Vec4(0.1f, 0.1f, 0.1f, 1), t);
	}
	else
	{
		stars_visibility = 0.f;
		clouds_color[0] = Vec4(1, 1, 1, 1);
	}
	clouds_color[1] = clouds_color[0];

	if(sun.enabled)
		sun.Update(time);
	if(moon.enabled)
	{
		moon.dir = Vec3(sin(time_period / 4 * PI * 2), 0.3f + sin(time * PI) / 4, cos(time_period / 4 * PI * 2));
		moon.color.w = stars_visibility;
	}
}

void Sky::CelestialObject::Update(float time)
{
	assert(orbit_normal.x != 0.f || orbit_normal.z != 0.f);

	Vec3 right = Vec3::Up.Cross(orbit_normal).Normalize();
	Vec3 up = orbit_normal.Cross(right).Normalize();

	float angle = time * (PI * 2 / period) + phase;
	dir = right * cosf(angle) + up * sinf(angle);
	dir.Normalize();
}

void Sky::CelestialObject::GetVertices(SimpleVertex(&v)[4]) const
{
	Vec3 up = dir.Cross(orbit_normal);
	Matrix rot = Matrix::CreateFromAxes(orbit_normal, up, dir);
	float delta = SKYDOME_RADIUS * tanf(size * 0.5f);

	v[0].pos = Vec3::Transform(Vec3(-delta, -delta, SKYDOME_RADIUS), rot);
	v[0].tex = Vec2(0.0f, 0.0f);

	v[1].pos = Vec3::Transform(Vec3(delta, -delta, SKYDOME_RADIUS), rot);
	v[1].tex = Vec2(1.0f, 0.0f);

	v[2].pos = Vec3::Transform(Vec3(-delta, delta, SKYDOME_RADIUS), rot);
	v[2].tex = Vec2(0.0f, 1.0f);

	v[3].pos = Vec3::Transform(Vec3(delta, delta, SKYDOME_RADIUS), rot);
	v[3].tex = Vec2(1.0f, 1.0f);
}
