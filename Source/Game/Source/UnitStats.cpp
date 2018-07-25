#include "GameCore.h"
#include "UnitStats.h"

const UnitStats UnitStats::stats[2] = {
	// walk	run		rot		alert	chase	can_run
	2.5f,	7.f,	5.f,	10.f,	15.f,	true,	// human
	1.5f,	0.f,	2.5f,	5.f,	10.f,	false,	// zombie
};
