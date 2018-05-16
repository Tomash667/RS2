#pragma once

class SoundLoader
{
public:
	SoundLoader(SoundManager* sound_mgr);
	Music* LoadMusic(cstring name, cstring path);
	Sound* LoadSound(cstring name, cstring path);

private:
	FMOD::Sound* Load(cstring path, bool is_music);

	FMOD::System* system;
};
