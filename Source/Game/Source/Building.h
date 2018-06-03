#pragma once

struct Building
{
	enum Door
	{
		DOOR_LEFT,
		DOOR_RIGHT,
		DOOR_BOTTOM,
		DOOR_TOP,
		DOOR_MAX
	};

	Box2d box;
	Int2 pos, size;
	int doors[DOOR_MAX];
};
