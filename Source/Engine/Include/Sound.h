#pragma once

#include "Resource.h"

struct Sound : public Resource
{
	Sound(cstring name, FMOD::Sound* snd) : Resource(name, Type::Sound) {}
	~Sound();

	FMOD::Sound* snd;
};

struct Music : public Resource
{
	Music(cstring name, FMOD::Sound* snd) : Resource(name, Type::Music) {}
	~Music();

	FMOD::Sound* snd;
};
