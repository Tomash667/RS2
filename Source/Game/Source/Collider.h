#pragma once

struct Collider
{
	Collider() {}
	Collider(const Vec2& center, const Vec2& half_size) : center(center), half_size(half_size) {}
	Box ToBox() const
	{
		return Box(center.x - half_size.x, 0, center.y - half_size.y,
			center.x + half_size.x, 4, center.y + half_size.y);
	}

	Vec2 center, half_size;
};
