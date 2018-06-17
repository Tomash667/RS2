#pragma once

struct Collider
{
	Collider() {}
	Collider(const Vec2& center, const Vec2& half_size, bool block_view = true) : center(center), half_size(half_size), block_view(block_view) {}
	Box2d ToBox2d() const
	{
		return Box2d(center.x - half_size.x, center.y - half_size.y,
			center.x + half_size.x, center.y + half_size.y);
	}
	Box ToBox() const
	{
		return Box(center.x - half_size.x, 0, center.y - half_size.y,
			center.x + half_size.x, 4, center.y + half_size.y);
	}

	Vec2 center, half_size;
	bool block_view;
};
