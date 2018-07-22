#include "GameCore.h"
#include "Npc.h"

void Npc::Save(FileWriter& f)
{
	Unit::Save(f);
	f << attack_player;
}

void Npc::Load(FileReader& f)
{
	Unit::Load(f);
	f >> attack_player;
}
