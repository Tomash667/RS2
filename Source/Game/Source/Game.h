#pragma once

#include <GameHandler.h>
#include "GameState.h"

class Game : public GameHandler
{
public:
	Game();
	~Game();
	int Start(cstring cmd_line);

private:
	void InitLogger();
	void InitEngine();
	void InitGame();
	void LoadResources();
	void StartGame(bool load = false);
	void ExitToMenu();
	bool OnTick(float dt) override;
	void UpdateGame(float dt);
	void UpdatePlayer(float dt);
	void UpdateUnits(float dt);
	void UpdateAlertPos(float dt);
	float UnitRotateTo(float& rot, float expected_rot, float speed, int* dir = nullptr);
	bool CheckForHit(Unit& unit, MeshPoint& hitbox, MeshPoint* bone, Unit*& target, Vec3& hitpoint);
	void HitUnit(Unit& unit, Unit& attacker, int dmg, const Vec3& hitpoint);
	bool CheckMove(Unit& uint, const Vec3& dir);
	bool CheckMovePos(Unit& unit, const Vec3& pos);
	void UnitAlert(Ai& ai, Unit* target, const Vec3* target_pos);
	void SearchForTarget(Ai& ai);
	bool CanSee(Unit& unit, const Vec3& pos);
	void OnDebugDraw(DebugDrawer* debug_drawer);
	void UpdateWorld(float dt);
	void SaveAndExit();
	void Save(FileWriter& f);
	void LoadGame();
	void Load(FileReader& f);
	void ShowErrorMessage(cstring err);
	void LoadConfig(cstring cmd_line);
	bool IsEnemy(Unit& u1, Unit& u2);

	unique_ptr<Engine> engine;
	Scene* scene;
	Input* input;
	ResourceManager* res_mgr;
	SoundManager* sound_mgr;
	Sky* sky;
	Config* config;
	GameState game_state;
	ThirdPersonCamera* camera;
	MainMenu* main_menu;
	GameGui* game_gui;
	PickPerkDialog* pick_perk;
	unique_ptr<Level> level;
	unique_ptr<CityGenerator> city_generator;
	unique_ptr<Navmesh> navmesh;
	vector<Unit*> targets;
	vector<std::pair<Vec3, float>> alert_pos;
	bool in_game, allow_mouse, quickstart, draw_navmesh;

	// debug pathfinding
#ifdef _DEBUG
	void UpdateTestPath(const Vec3& player_pos);
	bool pf_start_set, pf_end_set;
	Vec3 pf_start, pf_end;
#endif

	// resources
	Texture* tex_blood, *tex_zombie_blood, *tex_hit_object;
	Sound* sound_human_alert, *sound_human_hurt, *sound_human_die, *sound_zombie_hurt, *sound_zombie_die, *sound_zombie_attack,
		*sound_zombie_alert, *sound_hit, *sound_medkit, *sound_eat, *sound_hungry, *sound_shoot, *sound_shoot_try, *sound_reload;
};
