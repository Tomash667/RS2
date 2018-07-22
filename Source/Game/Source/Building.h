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
		bool IsConnected(uint index) const;
		Rect GetRect() const { return Rect::Create(pos, Int2(size.x - 1, size.y - 1)); }

		Int2 pos, size;
		int outside;

		// used only for generation - don't save
		vector<uint> connected, connected2;
		int outside_used;
		bool visited;
	};

	struct Table
	{
		Int2 pos;
		bool rotated;
	};

	Building() : mesh(nullptr) {}
	~Building() { delete mesh; }
	void CheckConnect(Room& room, uint index, uint& last_index);
	void SetIsDoorMap();
	bool IsDoor(const Int2& pt, DIR dir) const;
	void Save(FileWriter& f);
	void Load(FileReader& f);

	Box2d box;
	Int2 pos, size;
	vector<Room> rooms;
	vector<std::pair<Int2, DIR>> doors;
	vector<Table> tables;
	vector<int> is_door;
	Mesh* mesh;
};
