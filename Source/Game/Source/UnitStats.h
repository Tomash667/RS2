#pragma once

struct UnitStats
{
	float walk_speed;
	float run_speed;
	float rot_speed;
	float alert_range;
	float chase_range;
	bool can_run;

	// 0 - human, 1 - zombie
	static const UnitStats stats[2];
};
