#pragma once

class Engine
{
public:
	Engine();
	~Engine();
	void Init(GameHandler* handler);
	void Run();
	void ShowError(cstring err);
	void OnChangeResolution(const Int2& wnd_size);

	Input* GetInput() { return input.get(); }
	Render* GetRender() { return render.get(); }
	Window* GetWindow() { return window.get(); }
	SoundManager* GetSoundManager() { return sound_mgr.get(); }
	ResourceManager* GetResourceManager() { return res_mgr.get(); }
	Scene* GetScene() { return scene.get(); }
	Gui* GetGui() { return gui.get(); }
	float GetFps() { return fps; }

private:
	GameHandler* handler;
	unique_ptr<Input> input;
	unique_ptr<Window> window;
	unique_ptr<Render> render;
	unique_ptr<SoundManager> sound_mgr;
	unique_ptr<ResourceManager> res_mgr;
	unique_ptr<Scene> scene;
	unique_ptr<Gui> gui;
	Timer timer;
	uint frames;
	float frame_time, fps;
};
