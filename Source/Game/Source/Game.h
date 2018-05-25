#pragma once

#include <GameHandler.h>
#include "Collider.h"

class Game : public GameHandler
{
public:
	Game();
	~Game();
	int Start();

private:
	void InitLogger();
	void InitEngine();
	void InitGame();
	void LoadResources();
	void StartGame();
	void ExitToMenu();
	bool OnTick(float dt) override;
	void UpdateGame(float dt);
	void UpdatePlayer(float dt);
	void UpdateZombies(float dt);
	float UnitRotateTo(float& rot, float expected_rot, float speed, int* dir = nullptr);
	bool CheckForHit(Unit& unit, MeshPoint& hitbox, MeshPoint* bone, Unit*& target, Vec3& hitpoint);
	void HitUnit(Unit& unit, int dmg, const Vec3& hitpoint);
	bool CheckMove(Unit& uint, const Vec3& dir);

	unique_ptr<Engine> engine;
	Scene* scene;
	Input* input;
	ResourceManager* res_mgr;
	SoundManager* sound_mgr;
	ThirdPersonCamera* camera;
	MainMenu* main_menu;
	GameGui* game_gui;
	unique_ptr<Level> level;
	unique_ptr<CityGenerator> city_generator;
	bool in_game, allow_mouse;

	// resources
	Texture* tex_blood, *tex_zombie_blood, *tex_hit_object;
	Sound* sound_player_hurt, *sound_player_die, *sound_zombie_hurt, *sound_zombie_die, *sound_zombie_attack, *sound_zombie_alert, *sound_hit, *sound_medkit,
		*sound_eat, *sound_hungry, *sound_shoot, *sound_shoot_try, *sound_reload;
};
