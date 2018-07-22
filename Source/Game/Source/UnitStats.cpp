#include "GameCore.h"
#include "UnitStats.h"

const UnitStats UnitStats::stats[2] = {
	// walk	run		rot		can_run
	2.5f,	7.f,	5.f,	true,	// human
	1.5f,	0.f,	2.5f,	false,	// zombie
};
