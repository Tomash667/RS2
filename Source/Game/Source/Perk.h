#pragma once

enum class PerkId
{
	Agile,
	Firearms,
	LightEater,
	Medic,
	Necrology,
	Strong,
	Tough,
	Max
};

struct Perk
{
	PerkId id;
	cstring name, desc;

	static Perk perks[];
	static const uint n_perks;
	static const int max_level = 5;
};
