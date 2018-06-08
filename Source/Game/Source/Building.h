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

/*struct Building
{
	struct Room
	{
		Int2 pos, size, split_pos, split_size;
		Room* child_rooms[2];
	};

	Box2d box;
	Int2 pos, size;
	vector<Int2> doors2;
};*/
