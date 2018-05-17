#include "EngineCore.h"
#include "SoundLoader.h"
#include "SoundManager.h"
#include "Sound.h"
#include <fmod.hpp>

SoundLoader::SoundLoader(SoundManager* sound_mgr) : system(sound_mgr->GetSystem())
{
}

Music* SoundLoader::LoadMusic(cstring name, cstring path)
{
	FMOD::Sound* snd = Load(path, true);
	return new Music(name, snd);
}

Sound* SoundLoader::LoadSound(cstring name, cstring path)
{
	FMOD::Sound* snd = Load(path, false);
	return new Sound(name, snd);
}

FMOD::Sound* SoundLoader::Load(cstring path, bool is_music)
{
	int flags = FMOD_HARDWARE | FMOD_LOWMEM;
	if(is_music)
		flags |= FMOD_2D | FMOD_LOOP_NORMAL;
	else
		flags |= FMOD_3D;
	FMOD::Sound* sound;
	FMOD_RESULT result = system->createStream(path, flags, nullptr, &sound);
	if(result != FMOD_OK)
		throw Format("Failed to load %s '%s' (%d).", is_music ? "music" : "sound", path, result);
	return sound;
}
