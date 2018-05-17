#include "EngineCore.h"
#include "Engine.h"
#include "GameHandler.h"
#include "Input.h"
#include "Window.h"
#include "Render.h"
#include "SoundManager.h"
#include "ResourceManager.h"
#include "Scene.h"
#include "Gui.h"

Engine::Engine() : handler(nullptr), input(new Input), window(new Window), render(new Render), sound_mgr(new SoundManager), res_mgr(new ResourceManager),
scene(new Scene), gui(new Gui), fps(0)
{
}

Engine::~Engine()
{
	Info("Engine: Cleanup.");
	delete Logger::Get();
}

void Engine::Init(GameHandler* handler)
{
	assert(handler);
	this->handler = handler;

	Info("Engine: Initializing.");

	if(!XMVerifyCPUSupport())
		throw "Unsupported CPU.";

	window->Init(input.get());
	render->Init(window.get());
	sound_mgr->Init();
	res_mgr->Init(render.get(), sound_mgr.get());
	scene->Init(render.get());
	gui->SetWindowSize(window->GetSize());
	gui->Init(render.get(), res_mgr.get(), input.get());
}

void Engine::Run()
{
	Info("Engine: Started game loop.");
	timer.Start();
	frames = 0;
	frame_time = 0.f;

	while(true)
	{
		if(!window->Update())
			return;

		float dt = timer.Tick();
		frames++;
		frame_time += dt;
		if(frame_time >= 1.f)
		{
			fps = frames / frame_time;
			frames = 0;
			frame_time = 0.f;
		}

		if(dt > 0.3f)
			dt = 0.3f;
		if(!handler->OnTick(dt))
			return;

		scene->Update(dt);

		render->BeginScene();
		scene->Draw();
		gui->Draw(scene->GetViewProjectionMatrix());
		render->EndScene();

		input->Update();
		gui->Update();
		sound_mgr->Update(dt);
	}
}

void Engine::ShowError(cstring err)
{
	Logger::Get()->Log(Logger::L_ERROR, err);
	Logger::Get()->Flush();
	window->ShowError(err);
}
