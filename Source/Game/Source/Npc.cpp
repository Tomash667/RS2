#include "GameCore.h"
#include "Npc.h"

static cstring alone_texts[] = {
	"What the hell is going on with this city!",
	"This must be dream!",
	"So many deaths!",
	"Zombies! This is not possible!"
};

static cstring talk_texts[] = {
	"Do you have any spare food?"
};

static cstring escape_texts[] = {
	"Go away zombie!",
	"Shit!",
	"I'm out of here!"
};

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

void Npc::Talk(TalkMode mode)
{

}

bool Npc::CanTalk()
{
	if(attack_player
		|| state != AI_IDLE
		|| idle == IDLE_TALK_ALONE
		|| (idle == IDLE_TALK && attack_index == 1))
		return false;
	return true;
}
