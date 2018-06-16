#pragma once

#include <Vertex.h>
#include <Mesh.h>

enum DIR
{
	DIR_LEFT,
	DIR_RIGHT,
	DIR_BOTTOM,
	DIR_TOP,
	DIR_MAX
};

enum DIR_FLAGS
{
	DIR_F_LEFT = 1 << DIR_LEFT,
	DIR_F_RIGHT = 1 << DIR_RIGHT,
	DIR_F_BOTTOM = 1 << DIR_BOTTOM,
	DIR_F_TOP = 1 << DIR_TOP
};

struct Building
{
	struct Room
	{
		bool IsConnected(uint index) const
		{
			for(uint index2 : connected2)
			{
				if(index2 == index)
					return true;
			}
			return false;
		}

		Int2 pos, size;
		vector<uint> connected, connected2;
		int outside, outside_used;
		bool visited;
	};

	Building() : mesh(nullptr) {}
	~Building() { delete mesh; }
	void CheckConnect(Room& room, uint index, uint& last_index)
	{
		if(index == last_index)
			return;
		last_index = index;
		for(uint index2 : room.connected)
		{
			if(index2 == index)
				return;
		}
		room.connected.push_back(index);
	}

	bool IsDoor(const Int2& pt, DIR dir) const
	{
		int f = is_door[pt.x + pt.y * (size.x * 2)];
		return IS_SET(f, 1 << dir);
	}

	Box2d box;
	Int2 pos, size;
	vector<Room> rooms;
	vector<std::pair<Int2, DIR>> doors;
	vector<int> is_door;
	Mesh* mesh;
};
