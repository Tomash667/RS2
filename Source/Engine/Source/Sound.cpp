#include "EngineCore.h"
#include "Sound.h"
#include <fmod.hpp>

Sound::~Sound()
{
	if(snd)
		snd->release();
}

Music::~Music()
{
	if(snd)
		snd->release();
}
