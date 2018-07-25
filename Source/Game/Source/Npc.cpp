#include "GameCore.h"
#include "Npc.h"

void Npc::Save(FileWriter& f)
{
	Ai::Save(f);
	if(IsAlive())
		f << attack_player;
}

void Npc::Load(FileReader& f)
{
	Ai::Load(f);
	if(IsAlive())
		f >> attack_player;
}
