#include "GameCore.h"
#include "Perk.h"

Perk Perk::perks[] = {
	PerkId::Agile, "Agile", "Increase running speed.",
	PerkId::Firearms, "Firearms", "Increase accuracy and damage when using firearms.",
	PerkId::LightEater, "Light eater", "Increase max satiation and value gained from eating food.",
	PerkId::Medic, "Medic", "Increase health recovery from medkits.",
	PerkId::Necrology, "Necrology", "Increase damage against zombies.",
	PerkId::Strong, "Strong", "Increase melee weapons damage.",
	PerkId::Tough, "Tough", "Increase max health."
};

const uint Perk::n_perks = countof(Perk::perks);
