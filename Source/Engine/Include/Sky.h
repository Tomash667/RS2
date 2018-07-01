#pragma once

#include "Vertex.h"

template<typename T>
struct RoundInterpolator
{
	typedef typename std::pair<float, T> Entry;

	void Clear()
	{
		entries.clear();
	}

	void Add(float t, const T& item)
	{
		assert(entries.empty() || t > entries.back().first);
		entries.push_back(Entry(t, item));
	}
	
	T Get(float t) const
	{
		for(uint i = 0, count = entries.size(); i < count; ++i)
		{
			const Entry& entry = entries[i];
			if(entry.first == t)
				return entry.second;
			else if(entry.first > t)
			{
				const Entry& prev = entries[i - 1];
				float local_t = (t - prev.first) / (entry.first - prev.first);
				return T::Lerp(prev.second, entry.second, local_t);
			}
		}
		assert(0);
		return entries.back().second;
	}

private:
	vector<Entry> entries;
};

struct ColorPair
{
	Vec4 colors[2];

	ColorPair(const Vec4& color1, const Vec4& color2) : colors{ color1, color2 } {}
	const Vec4& operator [] (int i) const
	{
		return colors[i];
	}
	static ColorPair Lerp(const ColorPair& a, const ColorPair& b, float t)
	{
		return ColorPair(Vec4::Lerp(a.colors[0], b.colors[0], t),
			Vec4::Lerp(a.colors[1], b.colors[1], t));
	}
};

struct Sky
{
	struct CelestialObject
	{
		void Update(float time);
		void GetVertices(SimpleVertex(&v)[4]) const;

		Texture* texture;
		Vec4 color;
		Vec3 dir, orbit_normal;
		float size, period, phase;
		bool enabled;
	};

	Sky(Scene* scene);
	void Update(float dt);
	
	Scene* scene;
	CelestialObject sun, moon;
	RoundInterpolator<ColorPair> sky_colors;
	Vec4 clouds_color[2], // 0-borders, 1-center
		horizon_color,
		zenith_color;
	Vec2 clouds_offset,
		wind_vec;
	float time, prev_time, time_period,
		clouds_scale_inv, // clouds scaling (1-big clouds, 2-medium, 4-small, 8-very small etc)
		clouds_sharpness, // how sharp are clouds edges (0-15)
		clouds_threshold, // how many clouds (0-100%, 0.6-50%, 1-almost none, 1.2-none)
		stars_visibility;
	Texture* tex_clouds_noise,
		*tex_stars;

	static const float SKYDOME_RADIUS;
};
