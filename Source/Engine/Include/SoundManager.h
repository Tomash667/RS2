#pragma once

class SoundManager
{
public:
	SoundManager();
	~SoundManager();
	void Init();
	void Update(float dt);
	void PlayMusic(Music* music);
	void PlaySound2d(Sound* sound);
	void PlaySound3d(Sound* sound, const Vec3& pos, float smin);
	void StopSounds();

	void SetListenerPosition(const Vec3& pos, const Vec3& dir, const Vec3& up = Vec3::Up);
	void SetSoundVolume(int volume);
	void SetMusicVolume(int volume);

	FMOD::System* GetSystem() { return system; }
	int GetSoundVolume() { return sound_volume; }
	int GetMusicVolume() { return music_volume; }

private:
	FMOD::System* system;
	FMOD::ChannelGroup* group_sounds, *group_music;
	FMOD::Channel* current_music;
	vector<FMOD::Channel*> fallbacks;
	int sound_volume, music_volume; // 0-100
	bool play_sound, play_music;
};
