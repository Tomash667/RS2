#include "GameCore.h"
#include "Game.h"
#include <Engine.h>
#include <Input.h>
#include <Scene.h>
#include <ResourceManager.h>
#include <SceneNode.h>
#include <Camera.h>
#include <Window.h>
#include <Render.h>
#include <ctime>
#include <MeshInstance.h>
#include <ParticleEmitter.h>
#include "GameGui.h"
#include "Player.h"
#include "Zombie.h"
#include "CityGenerator.h"
#include "GroundItem.h"
#include "Level.h"
#include "Item.h"
#include "Version.h"
#include <SoundManager.h>
#include "ThirdPersonCamera.h"
#include "MainMenu.h"
#include "GameState.h"
#include <Gui.h>
#include <Config.h>
#include <Sky.h>
#include "PickPerkDialog.h"
#include "Perk.h"
#include "Navmesh.h"


const int level_size = 32;


Game::Game() : camera(nullptr), quickstart(false), config(nullptr), pick_perk(nullptr), draw_navmesh(false)
{
}

Game::~Game()
{
	delete camera;
	delete config;
	delete pick_perk;
}

int Game::Start(cstring cmd_line)
{
	engine.reset(new Engine);

	InitLogger();
	LoadConfig(cmd_line);

	try
	{
		InitEngine();
	}
	catch(cstring err)
	{
		engine->ShowError(Format("Failed to initialize engine: %s", err));
		return 1;
	}

	try
	{
		InitGame();
	}
	catch(cstring err)
	{
		engine->ShowError(Format("Failed to initialize game: %s", err));
		return 2;
	}

	// update config if required
	config->Save();

	try
	{
		engine->Run();
	}
	catch(cstring err)
	{
		engine->ShowError(Format("Fatal error when running game: %s", err));
		return 3;
	}
	catch(...)
	{
		engine->ShowError("Fatal unknown error when running game.");
		return 3;
	}

	return 0;
}

void Game::InitLogger()
{
	// log to text file and to console in debug build
	FileLogger* file_logger = new FileLogger("log.txt");
	if(DEBUG_BOOL)
	{
		MultiLogger* multi_logger = new MultiLogger;
		multi_logger->Add(file_logger);
		multi_logger->Add(new ConsoleLogger);
		Logger::Set(multi_logger);
	}
	else
		Logger::Set(file_logger);

	// log start
	time_t t = time(0);
	tm t2;
	localtime_s(&t2, &t);
	Info("RS v" VERSION_STR);
	Info("Date: %04d-%02d-%02d", t2.tm_year + 1900, t2.tm_mon + 1, t2.tm_mday);
}

void Game::InitEngine()
{
	Info("Initializing engine.");

	Window* window = engine->GetWindow();
	window->SetTitle("Rouge Survival v" VERSION_STR);
	window->SetFullscreen(config->GetBool("fullscreen"));
	window->SetSize(config->GetInt2("resolution"));
	engine->GetRender()->SetVsync(config->GetBool("vsync"));

	engine->Init(this);

	scene = engine->GetScene();
	input = engine->GetInput();
	res_mgr = engine->GetResourceManager();
	sound_mgr = engine->GetSoundManager();
	sound_mgr->SetSoundVolume(config->GetInt("volume"));
}

void Game::InitGame()
{
	Info("Initializing game.");
	in_game = false;

	Srand();
	engine->GetWindow()->SetCursorLock(true);

	level.reset(new Level);
	level->Init(scene, res_mgr, &game_state, CityGenerator::tile_size * level_size);
	game_state.level = level.get();
	game_state.engine = engine.get();
	game_state.config = config;

	navmesh.reset(new Navmesh);

	// sky
	sky = new Sky(scene);
	sky->tex_clouds_noise = res_mgr->GetTexture("sky/noise.png");
	sky->tex_stars = res_mgr->GetTexture("sky/stars.png");
	sky->sun.texture = res_mgr->GetTexture("sky/sun.png");
	sky->sun.enabled = true;
	sky->moon.texture = res_mgr->GetTexture("sky/moon.png");
	sky->moon.enabled = true;
	scene->SetSky(sky);

	//scene->SetDebugDrawHandler(delegate<void(DebugDrawer*)>(this, &Game::OnDebugDraw));
	//scene->SetDebugDrawEnabled(true);

	camera = new ThirdPersonCamera(scene->GetCamera(), level.get(), input);
#ifdef _DEBUG
	camera->cam->zfar = 50.f;
#endif

	LoadResources();

	city_generator.reset(new CityGenerator);
	city_generator->Init(scene, level.get(), res_mgr, level_size, 3, navmesh.get());

	main_menu = new MainMenu;
	main_menu->Init(res_mgr, &game_state);

	game_gui = new GameGui;
	game_gui->Init(engine.get(), &game_state, main_menu->options);
	game_gui->visible = false;

	pick_perk = new PickPerkDialog(&game_state, res_mgr);
}

void Game::LoadResources()
{
	// particle texture
	tex_blood = res_mgr->GetTexture("particles/blood.png");
	tex_zombie_blood = res_mgr->GetTexture("particles/zombie_blood.png");
	tex_hit_object = res_mgr->GetTexture("particles/hit_object.png");

	// sounds
	sound_player_hurt = res_mgr->GetSound("sounds/player_hurt.mp3");
	sound_player_die = res_mgr->GetSound("sounds/player_die.mp3");
	sound_zombie_hurt = res_mgr->GetSound("sounds/zombie_hurt.mp3");
	sound_zombie_die = res_mgr->GetSound("sounds/zombie_die.mp3");
	sound_zombie_attack = res_mgr->GetSound("sounds/zombie attack.wav");
	sound_zombie_alert = res_mgr->GetSound("sounds/zombie alert.wav");
	sound_hit = res_mgr->GetSound("sounds/hit.mp3");
	sound_medkit = res_mgr->GetSound("sounds/medkit.mp3");
	sound_eat = res_mgr->GetSound("sounds/eat.mp3");
	sound_hungry = res_mgr->GetSound("sounds/hungry.mp3");
	sound_shoot = res_mgr->GetSound("sounds/shoot.mp3");
	sound_shoot_try = res_mgr->GetSound("sounds/shoot_try.mp3");
	sound_reload = res_mgr->GetSound("sounds/reload.mp3");

	level->LoadResources();
	Item::LoadData(res_mgr);
}

void Game::StartGame(bool load)
{
	main_menu->Hide();
	game_gui->visible = true;
	if(!load)
	{
		Info("Starting new game.");
		city_generator->Generate();
		camera->reset = true;
		game_state.day = 0;
		game_state.last_hour = 16;
		game_state.hour = 16.50f; // 16:30
		sky->time = game_state.hour / 24;
		sky->prev_time = sky->time;
		sky->time_period = sky->time;
	}
	game_state.SetPaused(false);
	in_game = true;
}

void Game::ExitToMenu()
{
	city_generator->WaitForNavmeshThread();
	main_menu->Show();
	game_gui->visible = false;
	city_generator->Reset();
	in_game = false;
}

bool Game::OnTick(float dt)
{
	GameState::ChangeState change_state = game_state.GetChangeState();

	if((input->Down(Key::Alt) && input->Pressed(Key::F4))
		|| change_state == GameState::QUIT)
	{
		city_generator->WaitForNavmeshThread();
		return false;
	}
	
	Window* window = engine->GetWindow();
	if(input->Down(Key::Alt) && input->Pressed(Key::Enter))
	{
		bool fullscreen = !window->IsFullscreen();
		window->SetFullscreen(fullscreen);
		config->SetBool("fullscreen", fullscreen);
		config->Save();
	}

	if(input->Pressed(Key::U))
		window->SetCursorLock(!window->IsCursorLocked());

	if(in_game)
	{
		if(change_state == GameState::SAVE_AND_EXIT)
			SaveAndExit();
		else if(change_state == GameState::EXIT_TO_MENU)
			ExitToMenu();
		else
			UpdateGame(dt);
	}
	else
	{
		if(change_state == GameState::CONTINUE)
			LoadGame();
		else if(change_state == GameState::NEW_GAME || quickstart)
		{
			quickstart = false;
			StartGame();
		}
	}

	return true;
}

void Game::UpdateGame(float dt)
{
	if(game_state.IsPaused())
		return;

	allow_mouse = !game_gui->IsMouseRequired();

	city_generator->CheckNavmeshGeneration();
	UpdatePlayer(dt);
	UpdateZombies(dt);
	camera->Update(dt, allow_mouse);
	level->Update(dt);
	UpdateWorld(dt);

#ifdef _DEBUG
	if(input->Pressed(Key::N0))
	{
		SceneNode* node = level->player->node;
		level->SpawnZombie(node->pos + Vec3(cos(node->rot.y) * 10, 0, sin(node->rot.y) * 10));
	}
	if(input->Pressed(Key::F8))
		draw_navmesh = !draw_navmesh;
#endif

	game_state.hour += dt / 60;
#ifdef _DEBUG
	if(input->Down(Key::Shift))
		game_state.hour += dt * 2;
#endif
	if(game_state.hour >= 24.f)
	{
		game_state.hour -= 24.f;
		++game_state.day;
	}
	sky->time = game_state.hour / 24;
	sky->Update(dt);

	scene->Update(dt);
}

void Game::UpdatePlayer(float dt)
{
	Player* player = level->player;
	if(player->hp <= 0)
	{
		if(player->dying && player->node->mesh_inst->GetEndResult(0))
		{
			player->dying = false;
			if(!player->death_starved)
				level->SpawnBlood(*player);
		}
		return;
	}

#ifdef _DEBUG
	UpdateTestPath(player->node->pos);
#endif
	
	// show gick perk dialog when survived night
	if(game_state.day != player->last_survived_day && game_state.hour >= 7.f)
	{
		pick_perk->Show(player);
		player->last_survived_day = game_state.day;
	}
#ifdef _DEBUG
	if(input->Pressed(Key::P))
		pick_perk->Show(player);
#endif

	player->last_damage -= dt;
	if((player->hungry_timer -= dt) <= 0.f)
	{
		player->hungry_timer = Player::hunger_timestep;
		FoodLevel prev_food_level = player->GetFoodLevel();
		player->food = max(player->food - 1, -10);
		FoodLevel new_food_level = player->GetFoodLevel();
		if(prev_food_level != new_food_level && new_food_level <= FL_HUNGRY)
			sound_mgr->PlaySound3d(sound_hungry, player->GetSoundPos(), 1.f);
		if(player->food < 0)
		{
			player->hp -= 1;
			if(player->hp <= 0)
			{
				player->animation = ANI_DIE;
				player->node->mesh_inst->Play("umiera", PLAY_ONCE | PLAY_STOP_AT_END | PLAY_PRIO3, 0);
				return;
			}
		}
	}

	Animation animation = ANI_STAND;

	// check items before player
	if(player->action != A_PICKUP)
	{
		const float pick_range = 2.f;
		float best_range = pick_range;
		GroundItem* best_item = nullptr;
		for(GroundItem& item : level->items)
		{
			float dist = Vec3::Distance2d(player->node->pos, item.pos);
			if(dist < best_range)
			{
				float angle = AngleDiff(player->node->rot.y, Vec3::Angle2d(player->node->pos, item.pos));
				if(angle < PI / 4)
				{
					best_item = &item;
					best_range = dist;
				}
			}
		}
		if(player->item_before && player->item_before != best_item)
		{
			player->item_before->node->tint = Vec4::One;
			player->item_before = nullptr;
		}
		if(best_item && best_item != player->item_before)
		{
			player->item_before = best_item;
			best_item->node->tint = Vec4(2, 2, 2, 1);
		}

		if(player->action == A_NONE && player->item_before && input->Pressed(Key::E))
		{
			player->action = A_PICKUP;
			player->action_state = 0;
			animation = ANI_ACTION;
			player->node->mesh_inst->Play("podnosi", PLAY_ONCE | PLAY_CLEAR_FRAME_END_INFO, 0);
		}
	}

	if(input->Pressed(Key::H))
		player->UseMedkit();
	if(input->Pressed(Key::R))
		player->Reload();
	if(input->Pressed(Key::N1))
		player->SwitchWeapon(true);
	if(input->Pressed(Key::N2))
		player->SwitchWeapon(false);

	if(player->action == A_NONE && allow_mouse && input->Down(Key::LeftButton) && player->use_melee)
	{
		// attack
		player->action = A_ATTACK;
		player->action_state = 0;
		player->node->mesh_inst->Play(Rand() % 2 == 0 ? "atak1" : "atak2", PLAY_ONCE | PLAY_CLEAR_FRAME_END_INFO, 1);
	}
	if(player->action == A_NONE && allow_mouse && !player->use_melee && input->Down(Key::RightButton))
	{
		player->action = A_AIM;
		player->node->mesh_inst->Play("aim", PLAY_MANUAL, 1);
		camera->SetAim(true);
	}

	bool can_run = true, can_move = true;
	switch(player->action)
	{
	case A_NONE:
		break;
	case A_USE_MEDKIT:
		can_run = false;
		if(player->action_state == 0)
		{
			sound_mgr->PlaySound3d(sound_medkit, player->GetSoundPos(), 2.f);
			player->action_state = 1;
		}
		if(player->node->mesh_inst->GetEndResult(1))
		{
			int medic_level = player->GetPerkLevel(PerkId::Medic);
			float gain = 0.5f + 0.1f * medic_level;
			player->hp = min(player->hp + int(gain * player->maxhp), player->maxhp);
			--player->medkits;
			player->action = A_NONE;
			player->weapon->visible = true;
		}
		break;
	case A_EAT:
		can_run = false;
		if(player->action_state == 0 && player->node->mesh_inst->GetProgress(1) >= 19.f / 70)
		{
			player->action_state = 1;
			sound_mgr->PlaySound3d(sound_eat, player->GetSoundPos(), 2.f);
			float gain = 0.25f;
			player->food = min(int(player->food + gain * player->maxfood), player->maxfood);
			--player->food_cans;
		}
		if(player->node->mesh_inst->GetEndResult(1))
		{
			player->action = A_NONE;
			player->weapon->visible = true;
		}
		break;
	case A_PICKUP:
		if(player->action_state == 0)
		{
			// rotate towards item
			float expected_rot = Vec3::Angle2d(player->node->pos, player->item_before->pos);
			UnitRotateTo(player->node->rot.y, expected_rot, Player::rot_speed * dt);

			if(player->node->mesh_inst->GetProgress(0) > 19.f / 34)
			{
				// pickup item
				switch(player->item_before->item->type)
				{
				case Item::MEDKIT:
					++player->medkits;
					break;
				case Item::FOOD:
					++player->food_cans;
					break;
				case Item::RANGED_WEAPON:
					if(!player->ranged_weapon)
					{
						player->ranged_weapon = player->item_before->item;
						player->current_ammo = 10;
					}
					else
						player->ammo += 10;
					break;
				case Item::AMMO:
					player->ammo += 20;
					break;
				case Item::MELEE_WEAPON:
					if(player->melee_weapon != player->item_before->item)
					{
						player->melee_weapon = player->item_before->item;
						player->weapon->mesh = player->melee_weapon->mesh;
					}
					break;
				}
				level->RemoveItem(player->item_before);
				player->action_state = 1;
				player->item_before = nullptr;
			}
		}
		else if(player->node->mesh_inst->GetEndResult(0))
			player->action = A_NONE;
		can_move = false;
		break;
	case A_ATTACK:
		if(player->action_state == 0)
		{
			if(player->node->mesh_inst->GetProgress(1) > 14.f / 20)
			{
				// check for hitted target
				Mesh::Point* hitbox = player->weapon->mesh->FindPoint("hit");
				Mesh::Point* bone = (Mesh::Point*)player->weapon->GetParentPoint();
				Vec3 hitpoint;
				Unit* target;
				if(CheckForHit(*player, *hitbox, bone, target, hitpoint))
				{
					float damage_mod = 1.f;
					int strong_level = player->GetPerkLevel(PerkId::Strong);
					damage_mod += 0.2f * strong_level;
					if(target->is_zombie)
					{
						int necrology_level = player->GetPerkLevel(PerkId::Necrology);
						damage_mod += 0.1f * necrology_level;
					}
					HitUnit(*target, int(damage_mod * player->melee_weapon->RandomValue()), hitpoint);
				}
				player->action_state = 1;
			}
		}
		else if(player->node->mesh_inst->GetEndResult(1))
			player->action = A_NONE;
		can_run = false;
		break;
	case A_RELOAD:
		can_run = false;
		if(player->action_state == 0)
		{
			sound_mgr->PlaySound3d(sound_reload, player->GetSoundPos(), 2.f);
			player->action_state = 1;
		}
		if(player->node->mesh_inst->GetEndResult(1))
		{
			uint ammo = min(player->ammo, 10u - player->current_ammo);
			player->current_ammo += ammo;
			player->ammo -= ammo;
			player->action = A_NONE;
			player->weapon->visible = true;
		}
		if(camera->aim && (!allow_mouse || !input->Down(Key::RightButton)))
			camera->SetAim(false);
		break;
	case A_AIM:
		can_run = false;
		player->shot_delay -= dt;
		if(!allow_mouse || !input->Down(Key::RightButton))
		{
			if(player->shot_delay <= 0.f)
			{
				player->action = A_NONE;
				player->node->mesh_inst->Deactivate(1);
				camera->SetAim(false);
			}
		}
		else
		{
			const Vec2 angle_limits = ThirdPersonCamera::c_angle_aim;
			float ratio = (1.f - (camera->rot.y - angle_limits.x) / (angle_limits.y - angle_limits.x)) * 0.8f + 0.1f;
			player->node->mesh_inst->SetProgress(1, Clamp(ratio, 0.f, 1.f));

			if(player->current_ammo == 0 && player->ammo != 0 && player->shot_delay <= 0.f)
				player->Reload();
			else if(input->Pressed(Key::LeftButton)
				&& player->shot_delay <= 0
				&& !player->node->mesh_inst->GetGroup(1).IsBlending())
			{
				if(player->current_ammo == 0)
				{
					// try to shoot
					player->shot_delay = 0.1f;
					sound_mgr->PlaySound3d(sound_shoot_try, player->GetShootPos(), 1.f);
				}
				else
				{
					Vec3 shoot_pos = player->GetShootPos();
					Vec3 shoot_from = camera->cam->from;
					Vec3 shoot_dir = (camera->cam->to - shoot_from).Normalize() * 100.f;
					Vec3 target_pos = shoot_from + shoot_dir * 100.f + RandomPointInsideSphere(player->aim * 20);
					shoot_dir = (target_pos - shoot_from).Normalize() * 100.f;

					Unit* target;
					float t;
					if(level->RayTest(shoot_from, shoot_dir, t, Level::COLLIDE_ALL, player, &target))
					{
						Vec3 hitpoint = shoot_from + shoot_dir * t;
						if(target)
						{
							// hit unit
							float damage_mod = 1.f;
							int firearms_level = player->GetPerkLevel(PerkId::Firearms);
							damage_mod += 0.2f * firearms_level;
							if(target->is_zombie)
							{
								int necrology_level = player->GetPerkLevel(PerkId::Necrology);
								damage_mod += 0.1f * necrology_level;
							}
							HitUnit(*target, int(damage_mod * player->ranged_weapon->RandomValue()), hitpoint);
						}
						else
						{
							// hit object
							sound_mgr->PlaySound3d(sound_hit, hitpoint, 2.f);

							ParticleEmitter* pe = ParticleEmitter::Get();
							pe->tex = tex_hit_object;
							pe->emision_interval = 0.01f;
							pe->life = 1.f;
							pe->particle_life = 0.5f;
							pe->emisions = 1;
							pe->spawn_min = 10;
							pe->spawn_max = 15;
							pe->max_particles = 15;
							pe->pos = hitpoint;
							pe->speed_min = Vec3(-1, 0, -1);
							pe->speed_max = Vec3(1, 1, 1);
							pe->pos_min = Vec3(-0.1f, -0.1f, -0.1f);
							pe->pos_max = Vec3(0.1f, 0.1f, 0.1f);
							pe->size = 0.2f;
							pe->op_size = POP_LINEAR_SHRINK;
							pe->alpha = 0.8f;
							pe->op_alpha = POP_LINEAR_SHRINK;
							scene->Add(pe);
						}
					}

					sound_mgr->PlaySound3d(sound_shoot, shoot_pos, 4.f);
					player->weapon->mesh_inst->Play("shoot", PLAY_NO_BLEND | PLAY_ONCE, 0);

					player->shot_delay = 0.1f;
					player->UpdateAim(10.f);
					--player->current_ammo;
				}
			}
		}
		break;
	}

	float expected_aim = 0.f;

	int mov = 0;
	if(can_move)
	{
		if(input->Down(Key::W))
			mov += 10;
		if(input->Down(Key::S))
			mov -= 10;
		if(input->Down(Key::A))
			mov -= 1;
		if(input->Down(Key::D))
			mov += 1;

		int mouse_x = input->GetMouseDif().x;
		if(mouse_x != 0 && allow_mouse)
		{
			float value = float(mouse_x) / 400;
			player->rot_buf -= value;
		}
		if(allow_mouse)
		{
			Int2 dif = input->GetMouseDif();
			player->UpdateAim(float(max(abs(dif.x), abs(dif.y))) / 50);
		}
		if(player->rot_buf != 0)
		{
			if(player->rot_buf > 0)
			{
				player->last_rot = 0.1f;
				animation = ANI_ROTATE_LEFT;
				float rot = Player::rot_speed * dt;
				if(player->rot_buf > rot)
				{
					player->node->rot.y += rot;
					player->rot_buf -= rot;
				}
				else
				{
					player->node->rot.y += player->rot_buf;
					player->rot_buf = 0;
				}
			}
			else if(player->rot_buf < 0)
			{
				player->last_rot = -0.1f;
				animation = ANI_ROTATE_RIGHT;
				float rot = Player::rot_speed * dt;
				if(-player->rot_buf > rot)
				{
					player->node->rot.y -= rot;
					player->rot_buf += rot;
				}
				else
				{
					player->node->rot.y += player->rot_buf;
					player->rot_buf = 0;
				}
			}
			player->node->rot.y = Clip(player->node->rot.y);
			player->rot_buf *= (1.f - dt * 5);
		}
		else if(player->last_rot != 0)
		{
			if(player->last_rot > 0)
			{
				animation = ANI_ROTATE_LEFT;
				player->last_rot -= dt;
				if(player->last_rot < 0)
					player->last_rot = 0;
			}
			else
			{
				animation = ANI_ROTATE_RIGHT;
				player->last_rot += dt;
				if(player->last_rot > 0)
					player->last_rot = 0;
			}
		}
	}
	if(mov != 0)
	{
		float dir;
		bool back = false;
		switch(mov)
		{
		case 1: // right
			dir = PI * 0 / 4;
			break;
		case 11: // forward right
			dir = PI * 1 / 4;
			break;
		case 10: // forward
			dir = PI * 2 / 4;
			break;
		case 9: // forward left
			dir = PI * 3 / 4;
			break;
		case -1: // left
			dir = PI * 4 / 4;
			break;
		case -11: // backward left
			dir = PI * 5 / 4;
			back = true;
			break;
		case -10: // backward
			dir = PI * 6 / 4;
			back = true;
			break;
		case -9: // backward right
			dir = PI * 7 / 4;
			back = true;
			break;
		}
		dir += player->node->rot.y - PI / 2;
		bool run = can_run && !back && !input->Down(Key::Shift);

		const float speed = run ? player->GetRunSpeed() : Player::walk_speed;
		if(CheckMove(*player, Vec3(cos(dir), 0, sin(dir)) * (speed * dt)))
			player->node->pos.y = city_generator->GetY(player->node->pos);
		if(back)
			animation = ANI_WALK_BACK;
		else
			animation = run ? ANI_RUN : ANI_WALK;
		expected_aim = max(expected_aim, run ? 25.f : 10.f);
	}

	float d = 1.0f - exp(log(0.5f) * 5.f *dt);
	player->UpdateAim((expected_aim - player->aim) * d);

	if(player->action == A_NONE && animation == ANI_STAND)
	{
		if((player->idle_timer -= dt) <= 0)
		{
			player->idle_timer_max = Random(2.5f, 4.f);
			player->idle_timer = player->idle_timer_max + 1.5f;
			animation = ANI_IDLE;
		}
	}
	else
		player->idle_timer = player->idle_timer_max;

	if(player->action != A_NONE && player->animation == ANI_ACTION)
		animation = player->animation;
	if(player->animation == ANI_IDLE && animation == ANI_STAND && !player->node->mesh_inst->GetEndResult(0))
		animation = ANI_IDLE;

	player->Update(animation);
	engine->GetSoundManager()->SetListenerPosition(player->GetSoundPos(),
		Vec3(sin(-player->node->rot.y + PI / 2), 0, cos(-player->node->rot.y + PI / 2)));
}

#ifdef _DEBUG
void Game::UpdateTestPath(const Vec3& player_pos)
{
	bool check = false;
	if(input->Pressed(Key::F5))
	{
		pf_start = player_pos;
		pf_start_set = true;
		check = true;
	}
	if(input->Pressed(Key::F6))
	{
		pf_end = player_pos;
		pf_end_set = true;
		check = true;
	}
	if(check && pf_start_set && pf_end_set)
		navmesh->FindTestPath(pf_start, pf_end, false);
}
#endif

void Game::UpdateZombies(float dt)
{
	Player* player = level->player;
	for(Zombie* zombie : level->zombies)
	{
		if(zombie->hp <= 0)
		{
			if(zombie->dying && zombie->node->mesh_inst->GetEndResult(0))
			{
				zombie->dying = false;
				zombie->death_timer = 0;
				level->SpawnBlood(*zombie);
				--level->alive_zombies;
			}
			continue;
		}

		enum Move
		{
			MOVE_NO,
			MOVE_ROTATE,
			MOVE_FORWARD,
			MOVE_BACK // no pf
		};

		Animation animation = ANI_STAND;
		Move move = MOVE_NO;

		if(zombie->state == AI_IDLE || zombie->state == AI_FALLOW)
			SearchForTarget(zombie);

		if(zombie->state == AI_IDLE)
		{
			zombie->timer -= dt;
			if(zombie->timer <= 0.f)
			{
				zombie->timer = Zombie::idle_timer.Random();
				zombie->idle = (IdleAction)(Rand() % 4);
				switch(zombie->idle)
				{
				case IDLE_NONE:
					break;
				case IDLE_ROTATE:
					{
						float angle = Clip(zombie->node->rot.y + Random(PI / 2, PI * 3 / 2));
						zombie->target_pos = zombie->node->pos + Vec3(cos(angle), 0, sin(angle));
					}
					break;
				case IDLE_WALK:
					{
						float angle = Random(0.f, PI * 2);
						float dist = Random(2.f, 5.f);
						zombie->target_pos = zombie->node->pos + Vec3(cos(angle) * dist, 0, sin(angle) * dist);
						zombie->start_pos = zombie->node->pos;
					}
					break;
				case IDLE_ANIM:
					animation = ANI_IDLE;
					break;
				}
			}
			else
			{
				switch(zombie->idle)
				{
				case IDLE_NONE:
					break;
				case IDLE_ROTATE:
					move = MOVE_ROTATE;
					break;
				case IDLE_WALK:
					if(Vec3::Distance2d(zombie->target_pos, zombie->node->pos) < 0.1f
						|| Vec3::Distance2d(zombie->start_pos, zombie->node->pos) >= 6.f)
						zombie->idle = IDLE_NONE;
					else
						move = MOVE_FORWARD;
					break;
				case IDLE_ANIM:
					if(zombie->node->mesh_inst->GetEndResult(0))
					{
						animation = ANI_STAND;
						zombie->idle = IDLE_NONE;
					}
					else
						animation = ANI_IDLE;
					break;
				}
			}
		}
		else if(zombie->state == AI_COMBAT)
		{
			if(player->hp <= 0.f)
			{
				// target is dead
				zombie->ChangeState(AI_IDLE);
				continue;
			}

			zombie->timer -= dt; // attack delay

			float dist = Vec3::Distance2d(zombie->node->pos, player->node->pos);
			if(dist >= 10.f || !CanSee(*zombie, player->node->pos))
			{
				zombie->timer2 += dt;
				if(zombie->timer2 >= 0.5f)
				{
					// lost target, go to last known pos
					zombie->ChangeState(AI_FALLOW);
				}
				else
				{
					zombie->target_pos = player->node->pos;
					move = MOVE_FORWARD;
				}
			}
			else
			{
				zombie->target_pos = player->node->pos;
				zombie->timer2 = 0.f;

				if(dist > 1.2f)
				{
					// move towards player
					move = MOVE_FORWARD;
				}
				else if(dist < 1.f)
				{
					// move away from player
					move = MOVE_BACK;
				}
				else
					move = MOVE_ROTATE;

				if(!zombie->attacking && dist < 1.5f && zombie->timer <= 0 && zombie->GetAngleDiff(player->node->pos) < PI / 4)
				{
					// attack
					zombie->attacking = true;
					zombie->animation = ANI_ACTION;
					zombie->timer = Random(1.5f, 2.5f);
					zombie->attack_index = Rand() % 2 == 0 ? 0 : 1;
					zombie->node->mesh_inst->Play(zombie->attack_index == 0 ? "atak1" : "atak2", PLAY_ONCE | PLAY_CLEAR_FRAME_END_INFO, 0);
					if(Rand() % 4)
						sound_mgr->PlaySound3d(sound_zombie_attack, zombie->GetSoundPos(), 2.f);
				}
			}
		}
		else
		{
			float dist = Vec3::Distance2d(zombie->node->pos, zombie->target_pos);
			if(dist < 0.1f)
			{
				// target lost
				zombie->ChangeState(AI_IDLE);
			}
			else
				move = MOVE_FORWARD;
		}

		// calculate path
		Vec3 move_pos = Vec3::Zero;
		static const float PF_USED_TIMER = 0.25f;
		static const float PF_NOT_GENERATED_TIMER = 0.5f;
		if(move == MOVE_FORWARD)
		{
			zombie->pf_timer -= dt;

			// calculate path if:
			// + not used yet
			// + generation failed and timer passed
			// + is used and timer passed and target moved
			if(zombie->pf_state == PF_NOT_USED
				|| (zombie->pf_state == PF_NOT_GENERATED && zombie->pf_timer <= 0.f)
				|| (zombie->pf_state == PF_USED && zombie->pf_timer <= 0.f && Vec3::Distance(zombie->target_pos, zombie->pf_target) > 0.1f))
			{
				if(navmesh->FindPath(zombie->node->pos, zombie->target_pos, zombie->path))
				{
					zombie->pf_state = PF_USED;
					zombie->pf_timer = PF_USED_TIMER;
					zombie->pf_target = zombie->target_pos;
					zombie->pf_index = 1;
				}
				else
				{
					zombie->pf_state = PF_NOT_GENERATED;
					zombie->pf_timer = PF_NOT_GENERATED_TIMER;
				}
			}

			if(zombie->pf_state == PF_USED)
				move_pos = zombie->path[zombie->pf_index];
			else
				move_pos = zombie->target_pos;
		}
		else
		{
			zombie->pf_state = PF_NOT_USED;
			move_pos = zombie->target_pos;
		}

		if(move != MOVE_NO)
		{
			// rotate towards target
			float required_rot = Vec3::Angle2d(zombie->node->pos, move_pos);
			int dir;
			float dif = UnitRotateTo(zombie->node->rot.y, required_rot, dt * Zombie::rot_speed, &dir);
			if(dir == 1)
				animation = ANI_ROTATE_LEFT;
			else if(dir == -1)
				animation = ANI_ROTATE_RIGHT;

			if(dif < PI / 4)
			{
				float dist = Vec3::Distance2d(zombie->node->pos, move_pos);
				if(move == MOVE_FORWARD)
				{
					// move forward
					float travel_dist = Zombie::walk_speed * dt;
					bool moved = false;
					while(true)
					{
						if(travel_dist >= dist && CheckMovePos(*zombie, move_pos))
						{
							moved = true;
							if(zombie->pf_state != PF_USED || zombie->pf_index + 1 >= (int)zombie->path.size())
							{
								// reached end of path
								zombie->pf_state = PF_NOT_USED; 
								break;
							}

							// moving to next point
							++zombie->pf_index;
							move_pos = zombie->path[zombie->pf_index];
							travel_dist -= dist;
							required_rot = Vec3::Angle2d(zombie->node->pos, move_pos);
							dif = AngleDiff(zombie->node->rot.y, required_rot);
							if(dif < PI / 4)
								dist = Vec3::Distance2d(zombie->node->pos, move_pos);
							else
								break;
						}
						else
						{
							Vec3 move_dir = Vec3(cos(required_rot), 0, sin(required_rot)) * travel_dist;
							if(CheckMove(*zombie, move_dir))
								moved = true;
							break;
						}
					}

					if(moved)
					{
						zombie->node->pos.y = city_generator->GetY(zombie->node->pos);
						animation = ANI_WALK;
					}
				}
				else if(move == MOVE_BACK)
				{
					// move backward
					Vec3 move_dir = Vec3(cos(required_rot), 0, sin(required_rot)) * Zombie::walk_speed * dt * -0.66f;
					if(CheckMove(*zombie, move_dir))
					{
						zombie->node->pos.y = city_generator->GetY(zombie->node->pos);
						animation = ANI_WALK_BACK;
					}
				}
			}
		}

		if(zombie->attacking && zombie->node->mesh_inst->GetEndResult(0))
		{
			// end of attack, check for hit
			Mesh::Point* hitbox = zombie->node->mesh->GetPoint(Format("hitbox%d", zombie->attack_index + 1));
			if(!hitbox)
				hitbox = zombie->node->mesh->FindPoint("hitbox");
			Vec3 hitpoint;
			Unit* target;
			if(CheckForHit(*zombie, *hitbox, nullptr, target, hitpoint))
				HitUnit(*target, Random(10, 15), hitpoint);
			zombie->attacking = false;
		}

		zombie->last_damage -= dt;
		if(zombie->attacking)
			animation = zombie->animation;
		zombie->Update(animation);
	}

	LoopRemove(alert_pos, [dt](std::pair<Vec3, float>& alert)
	{
		return (alert.second -= dt) <= 0.f;
	});
}

void Game::SearchForTarget(Zombie* zombie)
{
	Player* player = level->player;
	if(player->hp > 0.f)
	{
		float dist = Vec3::Distance2d(zombie->node->pos, player->node->pos);
		if(dist <= 5.f)
		{
			float dif = zombie->GetAngleDiff(player->node->pos);
			if((dist <= 2.5f || dif <= PI / 2) && CanSee(*zombie, player->node->pos))
			{
				ZombieAlert(zombie);
				zombie->target_pos = player->node->pos;
				return;
			}
		}

		for(auto& alert : alert_pos)
		{
			dist = Vec3::Distance2d(zombie->node->pos, alert.first);
			if(dist <= 5.f && CanSee(*zombie, alert.first))
			{
				ZombieAlert(zombie, false);
				zombie->target_pos = alert.first;
				return;
			}
		}
	}
}

float Game::UnitRotateTo(float& rot, float expected_rot, float speed, int* dir)
{
	float dif = AngleDiff(rot, expected_rot);
	if(dif < speed)
	{
		rot = expected_rot;
		if(dir)
			*dir = 0;
		return 0;
	}
	else
	{
		float arc = ShortestArc(rot, expected_rot);
		rot = Clip(rot + Sign(arc) * speed);
		if(dir)
		{
			if(arc > 0)
				*dir = 1;
			else
				*dir = -1;
		}
		return dif - speed;
	}
}

bool Game::CheckForHit(Unit& unit, MeshPoint& mhitbox, MeshPoint* mbone, Unit*& target, Vec3& hitpoint)
{
	Mesh::Point& hitbox = (Mesh::Point&)mhitbox;
	Mesh::Point* bone = (Mesh::Point*)mbone;
	unit.node->mesh_inst->SetupBones();

	// calculate hitbox matrix
	Matrix m = Matrix::RotationY(-unit.node->rot.y) * Matrix::Translation(unit.node->pos); // m (World) = Rot * Pos
	if(bone)
		m = hitbox.mat * (bone->mat * unit.node->mesh_inst->GetMatrixBones()[bone->bone] * m);
	else
		m = hitbox.mat * unit.node->mesh_inst->GetMatrixBones()[hitbox.bone] * m;

	// create oriented box
	// a - weapon hitbox, b - unit hitbox
	Oob a, b;
	m._11 = m._22 = m._33 = 1.f;
	a.c = Vec3::TransformZero(m);
	a.e = hitbox.size;
	a.u[0] = Vec3(m._11, m._12, m._13);
	a.u[1] = Vec3(m._21, m._22, m._23);
	a.u[2] = Vec3(m._31, m._32, m._33);
	b.u[0] = Vec3(1, 0, 0);
	b.u[1] = Vec3(0, 1, 0);
	b.u[2] = Vec3(0, 0, 1);

	if(!unit.is_zombie)
	{
		// try hit zombie
		for(Zombie* zombie : level->zombies)
		{
			if(zombie->hp <= 0 || Vec3::Distance(unit.node->pos, zombie->node->pos) > 5.f)
				continue;

			Box box = zombie->GetBox();
			b.c = box.Midpoint();
			b.e = box.Size() / 2;

			if(Oob::Collide(b, a))
			{
				hitpoint = a.c;
				target = zombie;
				return true;
			}
		}
	}
	else
	{
		// try hit player
		Player* player = level->player;
		if(player->hp > 0 && Vec3::Distance(unit.node->pos, player->node->pos) <= 5.f)
		{
			Box box = player->GetBox();
			b.c = box.Midpoint();
			b.e = box.Size() / 2;

			if(Oob::Collide(b, a))
			{
				hitpoint = a.c;
				target = player;
				return true;
			}
		}
	}

	return false;
}

void Game::HitUnit(Unit& unit, int dmg, const Vec3& hitpoint)
{
	sound_mgr->PlaySound3d(sound_hit, hitpoint, 2.f);

	unit.hp -= dmg;
	if(unit.hp <= 0)
	{
		unit.animation = ANI_DIE;
		unit.dying = true;
		unit.node->mesh_inst->Play("umiera", PLAY_ONCE | PLAY_STOP_AT_END | PLAY_PRIO3 | PLAY_CLEAR_FRAME_END_INFO, 0);
		sound_mgr->PlaySound3d(unit.is_zombie ? sound_zombie_die : sound_player_die, unit.GetSoundPos(), 2.f);
		if(!unit.is_zombie)
			((Player&)unit).death_starved = false;
	}
	else
	{
		if(unit.last_damage <= 0.f && Rand() % 3 == 0)
			sound_mgr->PlaySound3d(unit.is_zombie ? sound_zombie_hurt : sound_player_hurt, unit.GetSoundPos(), 2.f);

		if(unit.is_zombie)
		{
			Zombie* zombie = (Zombie*)&unit;
			if(zombie->state == AI_IDLE)
				ZombieAlert(zombie);
		}
	}

	// add blood particle
	ParticleEmitter* pe = ParticleEmitter::Get();
	pe->tex = (unit.is_zombie ? tex_zombie_blood : tex_blood);
	pe->emision_interval = 0.01f;
	pe->life = 5.f;
	pe->particle_life = 0.5f;
	pe->emisions = 1;
	pe->spawn_min = 10;
	pe->spawn_max = 15;
	pe->max_particles = 15;
	pe->pos = hitpoint;
	pe->speed_min = Vec3(-1, 0, -1);
	pe->speed_max = Vec3(1, 1, 1);
	pe->pos_min = Vec3(-0.1f, -0.1f, -0.1f);
	pe->pos_max = Vec3(0.1f, 0.1f, 0.1f);
	pe->size = 0.3f;
	pe->op_size = POP_LINEAR_SHRINK;
	pe->alpha = 0.9f;
	pe->op_alpha = POP_LINEAR_SHRINK;
	scene->Add(pe);
}

bool Game::CheckMove(Unit& unit, const Vec3& dir)
{
	// move x, z
	Vec3 pos = unit.node->pos + dir;
	if(level->CheckCollision(unit, pos))
	{
		unit.node->pos = pos;
		return true;
	}

	// move x
	pos = unit.node->pos + Vec3(dir.x, 0, 0);
	if(level->CheckCollision(unit, pos))
	{
		unit.node->pos = pos;
		return true;
	}

	// move z
	pos = unit.node->pos + Vec3(0, 0, dir.z);
	if(level->CheckCollision(unit, pos))
	{
		unit.node->pos = pos;
		return true;
	}

	return false;
}

bool Game::CheckMovePos(Unit& unit, const Vec3& pos)
{
	if(level->CheckCollision(unit, pos))
	{
		unit.node->pos = pos;
		return true;
	}
	return false;
}

void Game::ZombieAlert(Zombie* zombie, bool first)
{
	if(first && zombie->state != AI_FALLOW)
	{
		sound_mgr->PlaySound3d(sound_zombie_alert, zombie->GetSoundPos(), 2.f);
		alert_pos.push_back({ zombie->node->pos, 1.f });
	}
	zombie->ChangeState(AI_COMBAT);
}

bool Game::CanSee(Unit& unit, const Vec3& pos)
{
	Vec3 from = unit.node->pos.ModY(1.5f);
	Vec3 to = pos.ModY(1.5f);
	Vec3 ray = to - from;
	float t;
	return !level->RayTest(from, ray, t, Level::COLLIDE_COLLIDERS | Level::COLLIDE_IGNORE_NO_BLOCK_VIEW, nullptr, nullptr);
}

void Game::OnDebugDraw(DebugDrawer* debug_drawer)
{
	if(draw_navmesh)
		navmesh->Draw(debug_drawer);

	for(Zombie* zombie : level->zombies)
	{
		if(zombie->IsAlive() && zombie->pf_state == PF_USED)
			navmesh->DrawPath(debug_drawer, zombie->path, false);
	}
}

void Game::UpdateWorld(float dt)
{
	int hour = (int)game_state.hour;
	if(hour == game_state.last_hour)
		return;

	game_state.last_hour = hour;

	// remove zombie corpses
	LoopRemove(level->zombies, [this](Zombie* zombie)
	{
		if(!zombie->IsAlive())
		{
			++zombie->death_timer;
			if(zombie->death_timer > 10)
			{
				zombie->node->pos.y -= 0.1f;
				if(zombie->death_timer >= 15)
				{
					scene->Remove(zombie->node);
					delete zombie;
					return true;
				}
			}
		}
		return false;
	});

	// remove blood
	LoopRemove(level->bloods, [this](Blood& blood)
	{
		++blood.timer;
		if(blood.timer >= 15)
		{
			blood.node->tint.w -= 0.1f;
			if(blood.node->tint.w <= 0.f)
			{
				scene->Remove(blood.node);
				return true;
			}
		}
		return false;
	});
	
	// spawn new zombies
	if(level->alive_zombies < 25)
	{
		int chance;
		if(game_state.hour <= 5.f || game_state.hour >= 19.f)
			chance = 1;
		else if(game_state.hour <= 8.f || game_state.hour >= 17.f)
			chance = 2;
		else
			chance = 4;

		if(Rand() % chance == 0 && Rand() % max(1, ((int)level->alive_zombies - 15)) == 0)
		{
			Vec2 player_pos = level->player->node->pos.XZ();
			for(int tries = 0; tries < 5; ++tries)
			{
				Vec2 pos = Vec2::Random(0, city_generator->GetMapSize());
				if(Vec2::Distance(player_pos, pos) < 15.f)
					continue;
				if(level->CheckCollision(nullptr, pos))
				{
					Vec3 spawn_pos(pos.x, 0, pos.y);
					spawn_pos.y = city_generator->GetY(spawn_pos);
					level->SpawnZombie(spawn_pos);
					break;
				}
			}
		}
	}
}

void Game::SaveAndExit()
{
	city_generator->WaitForNavmeshThread();

	try
	{
		FileWriter f("save");
		Save(f);
	}
	catch(cstring err)
	{
		ShowErrorMessage(Format("Failed to save game.\n%s", err));
		return;
	}

	Info("Saved game.");
	ExitToMenu();
}

void Game::Save(FileWriter& f)
{
	if(!f)
		throw "Failed to open file.";

	// header
	byte sign[] = { 'R','S','A','V' };
	f << sign;
	f << VERSION;

	// game state
	f << game_state.day;
	f << game_state.hour;
	f << game_state.last_hour;
	f << sky->time_period;

	// level
	level->Save(f);
	city_generator->Save(f);
	f << alert_pos;

	// camera
	camera->Save(f);
}

void Game::LoadGame()
{
	try
	{
		FileReader f("save");
		Load(f);
	}
	catch(cstring err)
	{
		ShowErrorMessage(Format("Failed to load game.\n%s", err));
		ExitToMenu();
		return;
	}

	Info("Loaded game.");
	io::DeleteFile("save");
	StartGame(true);
}

cstring VersionToString(int version)
{
	int major = (version & 0xFF0000) >> 16;
	int minor = (version & 0xFF00) >> 8;
	int patch = version & 0xFF;

	if(patch == 0)
		return Format("%d.%d", major, minor);
	else
		return Format("%d.%d.%d", major, minor, patch);
}

void Game::Load(FileReader& f)
{
	if(!f)
		throw "Failed to open file.";

	// file sign
	byte sign[] = { 'R','S','A','V' };
	byte sign2[4];
	f >> sign2;
	if(memcmp(sign, sign2, sizeof(sign)) != 0)
		throw "Invalid save header.";

	// version
	int version;
	f >> version;
	version &= 0xFFFFFF;
	if(version != VERSION)
		throw Format("Invalid save version %s.", VersionToString(version));

	// game state
	f >> game_state.day;
	f >> game_state.hour;
	f >> game_state.last_hour;
	sky->time = game_state.hour / 24;
	sky->prev_time = sky->time;
	f >> sky->time_period;

	// level
	level->Load(f);
	city_generator->Load(f);
	f >> alert_pos;

	// camera
	camera->Load(f);

	if(!f)
		throw "Broken file.";
}

void Game::ShowErrorMessage(cstring err)
{
	engine->GetGui()->ShowMessageBox(err);
}

void Game::LoadConfig(cstring cmd_line)
{
	string config_file = "rs.config";

	// parse command line
	vector<string> cmds;
	Config::SplitCommandLine(cmd_line, cmds);
	for(uint i = 0; i < cmds.size(); ++i)
	{
		const string& str = cmds[i];
		if(str[0] != '-')
			continue;
		if(str == "-config")
		{
			if(i + 1 < cmds.size())
			{
				++i;
				config_file = cmds[i];
			}
			else
				Warn("Missing command line argument for '-config'.");
		}
		else if(str, "-qs")
			quickstart = true;
		else
			Warn("Unknown command line switch '%s'.", str.c_str());
	}

	// load config
	Info("Using '%s' configuration file.", config_file.c_str());
	config = new Config(config_file);
	config->Add(new Config::Var("fullscreen", true));
	config->Add(new Config::Var("vsync", true));
	config->Add(new Config::Var("resolution", Int2::Zero));
	config->Add(new Config::Var("volume", 100));
	config->Load();
	config->ParseCommandLine(cmd_line);

	// validate settings
	Int2 res = config->GetInt2("resolution");
	Int2 valid_res = engine->GetRender()->CheckResolution(res);
	if(res != valid_res)
		config->SetInt2("resolution", valid_res);

	int volume = config->GetInt("volume");
	int valid_volume = Clamp(volume, 0, 100);
	if(volume != valid_volume)
		config->SetInt("volume", valid_volume);
}
