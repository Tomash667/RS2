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


const float Zombie::walk_speed = 1.5f;
const float Zombie::rot_speed = 2.5f;
const int level_size = 32;


Game::Game() : camera(nullptr)
{
}

Game::~Game()
{
	delete camera;
}

int Game::Start()
{
	engine.reset(new Engine);

	InitLogger();

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
	engine->GetWindow()->SetTitle("Rouge Survival v" VERSION_STR);
	engine->Init(this);

	scene = engine->GetScene();
	input = engine->GetInput();
	res_mgr = engine->GetResourceManager();
	sound_mgr = engine->GetSoundManager();
}

void Game::InitGame()
{
	in_game = false;

	Srand();
	engine->GetWindow()->SetCursorLock(true);

	level.reset(new Level);
	level->Init(scene, res_mgr, &game_state, CityGenerator::tile_size * level_size);
	game_state.level = level.get();

	// fog
	engine->GetRender()->SetClearColor(Color(200, 200, 200));
	scene->SetFogColor(Color(200, 200, 200));
	scene->SetFogParams(5.f, 20.f);

	camera = new ThirdPersonCamera(scene->GetCamera(), level.get(), input);

	LoadResources();

	city_generator.reset(new CityGenerator);
	city_generator->Init(scene, level.get(), res_mgr, level_size, 3);

	main_menu = new MainMenu;
	main_menu->Init(res_mgr, &game_state);

	game_gui = new GameGui;
	game_gui->Init(engine.get(), &game_state);
	game_gui->visible = false;
}

void Game::LoadResources()
{
	// particle texture
	tex_blood = res_mgr->GetTexture("blood.png");
	tex_zombie_blood = res_mgr->GetTexture("zombie_blood.png");
	tex_hit_object = res_mgr->GetTexture("hit_object.png");

	// sounds
	sound_player_hurt = res_mgr->GetSound("player_hurt.mp3");
	sound_player_die = res_mgr->GetSound("player_die.mp3");
	sound_zombie_hurt = res_mgr->GetSound("zombie_hurt.mp3");
	sound_zombie_die = res_mgr->GetSound("zombie_die.mp3");
	sound_zombie_attack = res_mgr->GetSound("zombie attack.wav");
	sound_zombie_alert = res_mgr->GetSound("zombie alert.wav");
	sound_hit = res_mgr->GetSound("hit.mp3");
	sound_medkit = res_mgr->GetSound("medkit.mp3");
	sound_eat = res_mgr->GetSound("eat.mp3");
	sound_hungry = res_mgr->GetSound("hungry.mp3");
	sound_shoot = res_mgr->GetSound("shoot.mp3");
	sound_shoot_try = res_mgr->GetSound("shoot_try.mp3");
	sound_reload = res_mgr->GetSound("reload.mp3");

	level->LoadResources();
	Item::LoadData(res_mgr);
}

void Game::StartGame()
{
	main_menu->Hide();
	game_gui->visible = true;
	city_generator->Generate();
	camera->reset = true;
	game_state.SetPaused(false);
	in_game = true;
}

void Game::ExitToMenu()
{
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
		return false;

	if(input->Pressed(Key::U))
		engine->GetWindow()->SetCursorLock(!engine->GetWindow()->IsCursorLocked());

	if(in_game)
	{
		if(change_state == GameState::EXIT_TO_MENU)
			ExitToMenu();
		else
			UpdateGame(dt);
	}
	else
	{
		if(change_state == GameState::CONTINUE)
		{
			// TODO
		}
		else if(change_state == GameState::NEW_GAME)
		{
			StartGame();
		}
	}

	return true;
}

void Game::UpdateGame(float dt)
{
	if(game_state.IsPaused())
		return;

	allow_mouse = !game_gui->IsInventoryOpen();

	UpdatePlayer(dt);
	UpdateZombies(dt);
	camera->Update(dt, allow_mouse);
	level->Update(dt);

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
			player->item_before->node->tint = Vec3::One;
			player->item_before = nullptr;
		}
		if(best_item && best_item != player->item_before)
		{
			player->item_before = best_item;
			best_item->node->tint = Vec3(2, 2, 2);
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
			player->hp = min(player->hp + 50, 100);
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
			player->food = min(player->food + 25, 100);
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
					HitUnit(*target, player->melee_weapon->RandomValue(), hitpoint);
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
							HitUnit(*target, player->ranged_weapon->RandomValue(), hitpoint);
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
					player->aim += 10.f;
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
			player->aim += float(max(abs(dif.x), abs(dif.y))) / 50;
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

		const float speed = run ? Player::run_speed : Player::walk_speed;
		if(CheckMove(*player, Vec3(cos(dir), 0, sin(dir)) * (speed * dt)))
			player->node->pos.y = city_generator->GetY(player->node->pos);
		if(back)
			animation = ANI_WALK_BACK;
		else
			animation = run ? ANI_RUN : ANI_WALK;
		expected_aim = max(expected_aim, run ? 25.f : 10.f);
	}

	float d = 1.0f - exp(log(0.5f) * 5.f *dt);
	player->aim += (expected_aim - player->aim) * d;

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
				level->SpawnBlood(*zombie);
			}
			continue;
		}

		if(player->hp <= 0)
		{
			zombie->Update(ANI_STAND);
			continue;
		}

		zombie->last_damage -= dt;

		// search for player
		float dist = Vec3::Distance2d(zombie->node->pos, player->node->pos);
		if(!zombie->active)
		{
			if(dist <= 5.f)
			{
				zombie->active = true;
				sound_mgr->PlaySound3d(sound_zombie_alert, zombie->GetSoundPos(), 2.f);
			}
			else
				continue;
		}

		Animation animation = ANI_STAND;

		// rotate towards player
		float required_rot = Vec3::Angle2d(zombie->node->pos, player->node->pos);
		int dir;
		float dif = UnitRotateTo(zombie->node->rot.y, required_rot, dt * Zombie::rot_speed, &dir);
		if(dir == 1)
			animation = ANI_ROTATE_LEFT;
		else if(dir == -1)
			animation = ANI_ROTATE_RIGHT;

		if(dif < PI / 4)
		{
			if(dist > 1.2f)
			{
				// move towards player
				Vec3 move_dir = Vec3(cos(required_rot), 0, sin(required_rot)) * Zombie::walk_speed * dt;
				if(CheckMove(*zombie, move_dir))
				{
					zombie->node->pos.y = city_generator->GetY(zombie->node->pos);
					animation = ANI_WALK;
				}
			}
			else if(dist < 1.f)
			{
				// move away player
				Vec3 move_dir = Vec3(cos(required_rot), 0, sin(required_rot)) * Zombie::walk_speed * dt * 0.66f;
				if(CheckMove(*zombie, move_dir))
				{
					zombie->node->pos.y = city_generator->GetY(zombie->node->pos);
					animation = ANI_WALK_BACK;
				}
			}

			if(!zombie->attacking && dist < 1.5f && zombie->next_attack <= 0)
			{
				zombie->attacking = true;
				zombie->animation = ANI_ACTION;
				zombie->next_attack = Random(1.5f, 2.5f);
				zombie->attack_index = Rand() % 2 == 0 ? 0 : 1;
				zombie->node->mesh_inst->Play(zombie->attack_index == 0 ? "atak1" : "atak2", PLAY_ONCE | PLAY_CLEAR_FRAME_END_INFO, 0);
				if(Rand() % 4)
					sound_mgr->PlaySound3d(sound_zombie_attack, zombie->GetSoundPos(), 2.f);
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

		zombie->next_attack -= dt;
		if(zombie->attacking)
			animation = zombie->animation;
		zombie->Update(animation);
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
			Zombie& zombie = (Zombie&)unit;
			if(!zombie.active)
			{
				zombie.active = true;
				sound_mgr->PlaySound3d(sound_zombie_alert, zombie.GetSoundPos(), 2.f);
			}
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
