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


const float Player::walk_speed = 2.5f;
const float Player::run_speed = 7.f;
const float Player::rot_speed = 4.f;
const float Zombie::walk_speed = 1.5f;
const float Zombie::rot_speed = 2.5f;
const int level_size = 32;


Game::Game()
{
}

Game::~Game()
{
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
	Info("RS v0");
	Info("Date: %04d-%02d-%02d", t2.tm_year + 1900, t2.tm_mon + 1, t2.tm_mday);
}

void Game::InitEngine()
{
	engine->GetWindow()->SetTitle("Rouge Savior V0");
	engine->Init(this);

	scene = engine->GetScene();
	input = engine->GetInput();
	res_mgr = engine->GetResourceManager();
	camera = scene->GetCamera();
}

void Game::InitGame()
{
	Srand();
	engine->GetWindow()->SetCursorLock(true);

	level.reset(new Level);
	level->Init(scene, res_mgr, CityGenerator::tile_size * level_size);

	// fog
	engine->GetRender()->SetClearColor(Color(200, 200, 200));
	scene->SetFogColor(Color(200, 200, 200));
	scene->SetFogParams(5.f, 20.f);

	// camera
	cam_rot = Vec2(0, 4.47908592f);
	cam_dist = 1.5f;
	cam_shift = 0.25f;

	LoadResources();
	GenerateCity();

	game_gui = new GameGui;
	game_gui->Init(engine.get(), level->player);
}

void Game::LoadResources()
{
	// particle texture
	tex_blood = res_mgr->GetTexture("blood.png");
	tex_zombie_blood = res_mgr->GetTexture("zombie_blood.png");
}

void Game::GenerateCity()
{
	city_generator.reset(new CityGenerator);
	city_generator->Init(scene, level.get(), res_mgr, level_size, 3);
	city_generator->Generate();
}

bool Game::OnTick(float dt)
{
	if(input->Pressed(Key::Escape) || (input->Down(Key::Alt) && input->Pressed(Key::F4)))
		return false;

	if(input->Pressed(Key::U))
		engine->GetWindow()->SetCursorLock(!engine->GetWindow()->IsCursorLocked());

	UpdatePlayer(dt);
	UpdateZombies(dt);
	UpdateCamera();
	game_gui->Update();

	return true;
}

void Game::UpdatePlayer(float dt)
{
	Player* player = level->player;
	if(player->hp <= 0)
		return;

	Animation animation = ANI_STAND;

	// check items before player
	if(player->action != A_PICKUP)
	{
		const float pick_range = 2.f;
		float best_range = pick_range;
		GroundItem* best_item = nullptr;
		for(GroundItem& item : level->items)
		{
			float dist = Vec3::Distance2d(player->node->pos, item.node->pos);
			if(dist < best_range)
			{
				float angle = AngleDiff(player->node->rot.y, Vec3::Angle2d(player->node->pos, item.node->pos));
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

	if(player->action == A_NONE)
	{
		if(player->hp != 100 && input->Pressed(Key::H) && player->medkits != 0)
		{
			// use medkit
			player->action = A_USE_MEDKIT;
			player->node->mesh_inst->Play("use", PLAY_ONCE | PLAY_CLEAR_FRAME_END_INFO, 1);
		}
		else if(input->Down(Key::LeftButton))
		{
			// attack
			player->action = A_ATTACK;
			player->action_state = 0;
			player->node->mesh_inst->Play("atak1", PLAY_ONCE | PLAY_CLEAR_FRAME_END_INFO, 1);
		}
	}

	bool can_run = true, can_move = true;
	switch(player->action)
	{
	case A_NONE:
		break;
	case A_USE_MEDKIT:
		can_run = false;
		if(player->node->mesh_inst->GetEndResult(1))
		{
			player->hp = min(player->hp + 50, 100);
			--player->medkits;
			player->action = A_NONE;
		}
		break;
	case A_PICKUP:
		if(player->action_state == 0)
		{
			// rotate towards item
			float expected_rot = Vec3::Angle2d(player->node->pos, player->item_before->node->pos);
			UnitRotateTo(player->node->rot.y, expected_rot, Player::rot_speed * dt);

			if(player->node->mesh_inst->GetProgress(0) > 19.f / 34)
			{
				// pickup item
				level->RemoveItem(player->item_before);
				player->action_state = 1;
				player->item_before = nullptr;
				++player->medkits;
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
				Mesh::Point* bone = player->node->mesh->GetPoint("bron");
				Vec3 hitpoint;
				Unit* target;
				if(CheckForHit(*player, *hitbox, bone, target, hitpoint))
					HitUnit(*target, Random(45, 55), hitpoint);
				player->action_state = 1;
			}
		}
		else if(player->node->mesh_inst->GetEndResult(1))
			player->action = A_NONE;
		can_run = false;
		break;
	}

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
		if(mouse_x != 0)
		{
			float value = float(mouse_x) / 400;
			player->rot_buf -= value;
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
	}

	if(player->action != A_NONE && player->animation == ANI_ACTION)
		animation = player->animation;

	player->Update(animation);
}

void Game::UpdateZombies(float dt)
{
	Player* player = level->player;
	for(Zombie* zombie : level->zombies)
	{
		if(zombie->hp <= 0)
			continue;

		if(player->hp <= 0)
		{
			zombie->Update(ANI_STAND);
			continue;
		}

		// search for player
		float dist = Vec3::Distance2d(zombie->node->pos, player->node->pos);
		if(!zombie->active)
		{
			if(dist <= 5.f)
				zombie->active = true;
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

void Game::UpdateCamera()
{
	Player* player = level->player;

	//if(input->Down(Key::Shift))
	//	cam_shift += 0.25f * input->GetMouseWheel();

	cam_dist -= 0.25f * input->GetMouseWheel();
	if(cam_dist < 0.25f)
		cam_dist = 0.25f;
	else if(cam_dist > 5.f)
		cam_dist = 5.f;
	if(input->Down(Key::MiddleButton))
	{
		cam_dist = 1.5f;
		cam_rot.y = 4.47908592f;
	}

	const Vec2 c_cam_angle = Vec2(PI + 0.1f, PI * 1.8f - 0.1f);
	cam_rot.x = player->node->rot.y;
	cam_rot.y = c_cam_angle.Clamp(cam_rot.y - float(input->GetMouseDif().y) / 400);

	const float cam_h = 1.7f;

	camera->to = player->node->pos;
	camera->to.y += cam_h;
	Vec3 camera_to_without_shift = camera->to;

	float shift = cam_shift * (cam_dist - 0.25f) / 1.25f;
	if(shift != 0)
	{
		float angle = -cam_rot.x + PI;
		camera->to += Vec3(sin(angle)*shift, 0, cos(angle)*shift);
	}

	Vec3 ray(0, -cam_dist, 0);
	Matrix mat = Matrix::Rotation(-cam_rot.x - PI / 2, cam_rot.y, 0);
	ray = Vec3::Transform(ray, mat);

	float t = min(level->RayTest(camera->to, ray), level->RayTest(camera_to_without_shift, ray));

	shift *= t;
	if(shift != 0)
	{
		float angle = -cam_rot.x + PI;
		camera->to = camera_to_without_shift + Vec3(sin(angle)*shift, 0, cos(angle)*shift);
	}
	camera->from = camera->to + ray * t;

	if(ray.Length() * t < 0.3f)
	{
		player->node->visible = false;
		player->hair->visible = false;
	}
	else
	{
		player->node->visible = true;
		player->hair->visible = true;
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
	unit.hp -= dmg;
	if(unit.hp <= 0)
	{
		unit.animation = ANI_DIE;
		unit.node->mesh_inst->Play("umiera", PLAY_ONCE | PLAY_STOP_AT_END | PLAY_PRIO3, 0);
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
