#pragma once

#include "Resource.h"

struct Sound : public Resource
{
	Sound(cstring name, FMOD::Sound* snd) : Resource(name, Type::Sound), snd(snd) {}
	~Sound();

	FMOD::Sound* snd;
};

struct Music : public Resource
{
	Music(cstring name, FMOD::Sound* snd) : Resource(name, Type::Music), snd(snd) {}
	~Music();

	FMOD::Sound* snd;
};
