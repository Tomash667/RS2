#pragma once

struct UnitStats
{
	float walk_speed;
	float run_speed;
	float rot_speed;
	bool can_run;

	// 0 - human, 1 - zombie
	static const UnitStats stats[2];
};
