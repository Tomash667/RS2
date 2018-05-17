#include "EngineCore.h"
#include "SoundManager.h"
#include "Sound.h"
#include <fmod.hpp>

SoundManager::SoundManager() : system(nullptr), current_music(nullptr), sound_volume(50), music_volume(50), play_sound(true), play_music(true)
{
}

SoundManager::~SoundManager()
{
	if(system)
		system->release();
}

void SoundManager::Init()
{
	FMOD_RESULT result = FMOD::System_Create(&system);
	if(result != FMOD_OK)
		throw Format("Failed to create FMOD system (%d).", result);

	result = system->init(128, FMOD_INIT_NORMAL, nullptr);
	if(result != FMOD_OK)
		throw Format("Failed to initialize FMOD system (%d).", result);

	system->createChannelGroup("sounds", &group_sounds);
	system->createChannelGroup("music", &group_music);
	group_sounds->setVolume(float(sound_volume) / 100);
	group_music->setVolume(float(music_volume) / 100);
}

void SoundManager::Update(float dt)
{
	// update fallbacks
	LoopRemove(fallbacks, [=](FMOD::Channel* channel)
	{
		float volume;
		channel->getVolume(&volume);
		if((volume -= dt) <= 0.f)
		{
			channel->stop();
			return true;
		}
		else
		{
			channel->setVolume(volume);
			return false;
		}
	});

	// update music
	if(current_music)
	{
		float volume;
		current_music->getVolume(&volume);
		if(volume != 1.f)
		{
			volume = min(1.f, volume + dt);
			current_music->setVolume(volume);
		}
	}

	system->update();
}

void SoundManager::PlayMusic(Music* music)
{
	if(music)
	{
		if(current_music)
		{
			FMOD::Sound* snd;
			current_music->getCurrentSound(&snd);
			if(snd == music->snd)
				return;
			fallbacks.push_back(current_music);
		}

		system->playSound(FMOD_CHANNEL_FREE, music->snd, true, &current_music);
		current_music->setVolume(0.f);
		current_music->setChannelGroup(group_music);
		current_music->setPaused(false);
	}
	else
	{
		if(!current_music)
			return;
		fallbacks.push_back(current_music);
		current_music = nullptr;
	}
}

void SoundManager::PlaySound2d(Sound* sound)
{
	assert(sound);
	if(!play_sound)
		return;

	FMOD::Channel* channel;
	system->playSound(FMOD_CHANNEL_FREE, sound->snd, true, &channel);
	channel->setMode(FMOD_2D);
	channel->setPaused(false);
	channel->setChannelGroup(group_sounds);
}

void SoundManager::PlaySound3d(Sound* sound, const Vec3& pos, float smin)
{
	assert(sound);
	if(!play_sound)
		return;

	FMOD::Channel* channel;
	system->playSound(FMOD_CHANNEL_FREE, sound->snd, true, &channel);
	channel->setMode(FMOD_3D);
	channel->set3DAttributes((const FMOD_VECTOR*)&pos, nullptr);
	channel->set3DMinMaxDistance(smin, 10000.f);
	channel->setPaused(false);
	channel->setChannelGroup(group_sounds);
}

void SoundManager::StopSounds()
{
	group_sounds->stop();
}

void SoundManager::SetListenerPosition(const Vec3& pos, const Vec3& dir, const Vec3& up)
{
	system->set3DListenerAttributes(0, (const FMOD_VECTOR*)&pos, nullptr, (const FMOD_VECTOR*)&dir, (const FMOD_VECTOR*)&up);
}

void SoundManager::SetSoundVolume(int volume)
{
	assert(InRange(volume, 0, 100));
	if(volume == sound_volume)
		return;
	sound_volume = volume;
	if(volume)
	{
		play_sound = true;
		group_sounds->setVolume(float(sound_volume) / 100);
	}
	else
	{
		play_sound = false;
		StopSounds();
	}
}

void SoundManager::SetMusicVolume(int volume)
{
	assert(InRange(volume, 0, 100));
	if(volume == music_volume)
		return;
	play_music = (volume > 0);
	group_music->setVolume(float(volume) / 100);
	music_volume = volume;
}
