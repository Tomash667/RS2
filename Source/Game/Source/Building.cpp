#include "GameCore.h"
#include "Building.h"

bool Building::Room::IsConnected(uint index) const
{
	for(uint index2 : connected2)
	{
		if(index2 == index)
			return true;
	}
	return false;
}

void Building::CheckConnect(Room& room, uint index, uint& last_index)
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

void Building::SetIsDoorMap()
{
	Int2 s = size * 2;
	is_door.resize(s.x * s.y, 0);
	for(std::pair<Int2, DIR>& door : doors)
	{
		is_door[door.first.x + door.first.y * s.x] |= 1 << door.second;
		switch(door.second)
		{
		case DIR_LEFT:
			if(door.first.x != 0)
				is_door[door.first.x - 1 + door.first.y * s.x] |= DIR_F_RIGHT;
			break;
		case DIR_RIGHT:
			if(door.first.x != s.x - 1)
				is_door[door.first.x + 1 + door.first.y * s.x] |= DIR_F_LEFT;
			break;
		case DIR_TOP:
			if(door.first.y != 0)
				is_door[door.first.x + (door.first.y - 1) * s.x] |= DIR_F_BOTTOM;
			break;
		case DIR_BOTTOM:
			if(door.first.y != s.y - 1)
				is_door[door.first.x + (door.first.y + 1) * s.x] |= DIR_F_TOP;
			break;
		}
	}
}

bool Building::IsDoor(const Int2& pt, DIR dir) const
{
	int f = is_door[pt.x + pt.y * (size.x * 2)];
	return IS_SET(f, 1 << dir);
}

void Building::Save(FileWriter& f)
{
	f << box;
	f << pos;
	f << size;
	f << rooms.size();
	for(Room& room : rooms)
	{
		f << room.pos;
		f << room.size;
		f << room.outside;
	}
	f << doors;
	f << tables;
}

void Building::Load(FileReader& f)
{
	f >> box;
	f >> pos;
	f >> size;
	rooms.resize(f.Read<uint>());
	for(Room& room : rooms)
	{
		f >> room.pos;
		f >> room.size;
		f >> room.outside;
	}
	f >> doors;
	f >> tables;
}
