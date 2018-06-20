#include "GameCore.h"
#include "Options.h"
#include "GameState.h"
#include <Engine.h>
#include <Window.h>
#include <Render.h>
#include <SoundManager.h>
#include <Gui.h>
#include <Input.h>
#include <Config.h>

Options::Options(GameState* game_state) : game_state(game_state)
{
	// TILE
	// [] Fullscreen
	// [] Vsync
	// [-----] Resolution
	// <----X> Volume
	//    [Ok]

	size = Int2(200, 200);

	cb_fullscreen = new CheckBox;
	Add(cb_fullscreen);

	cb_vsync = new CheckBox;
	Add(cb_vsync);

	ddl_resolution = new DropDownList;
	const vector<Int2>& resolutions = game_state->engine->GetRender()->GetAvailableResolutions();
	ddl_resolution->items.resize(resolutions.size());
	for(uint i = 0; i < resolutions.size(); ++i)
	{
		DropDownList::Item& item = ddl_resolution->items[i];
		Int2 res = resolutions[i];
		item.text = Format("%d x %d", res.x, res.y);
		item.value = i;
	}
	Add(ddl_resolution);

	sl_volume = new Slider;
	sl_volume->min_value = 0;
	sl_volume->max_value = 100;
	sl_volume->step = 1;
	Add(sl_volume);

	Button* bt = new Button;
	bt->text = "OK";
	bt->event = delegate<void(int)>(this, &Options::OnEvent);
	bt->size = Int2(100, 30);
	bt->SetPos(Int2((size.x - bt->size.x) / 2, size.y - bt->size.y - 10));
	Add(bt);

	visible = false;
}

void Options::Draw()
{
	Panel::Draw();
}

void Options::Update(float dt)
{
	if(gui->GetInput()->PressedOnce(Key::Escape))
	{
		Close();
		return;
	}

	Window* window = game_state->engine->GetWindow();
	Render* render = game_state->engine->GetRender();
	SoundManager* sound_mgr = game_state->engine->GetSoundManager();

	// handle alt enter change
	bool current_fullscreen = window->IsFullscreen();
	if(current_fullscreen != fullscreen)
	{
		fullscreen = current_fullscreen;
		cb_fullscreen->checked = fullscreen;
	}

	if(fullscreen != cb_fullscreen->checked)
	{
		fullscreen = cb_fullscreen->checked;
		window->SetFullscreen(fullscreen);
	}

	if(vsync != cb_vsync->checked)
	{
		vsync = cb_vsync->checked;
		render->SetVsync(vsync);
	}

	if(resolution_index != ddl_resolution->selected_index)
	{
		resolution_index = ddl_resolution->selected_index;
		window->SetSize(render->GetAvailableResolutions()[resolution_index]);
	}

	if(volume != sl_volume->value)
	{
		volume = sl_volume->value;
		sound_mgr->SetSoundVolume(volume);
	}

	Panel::Update(dt);
}

void Options::Show()
{
	Window* window = game_state->engine->GetWindow();
	Render* render = game_state->engine->GetRender();
	SoundManager* sound_mgr = game_state->engine->GetSoundManager();

	fullscreen = window->IsFullscreen();
	cb_fullscreen->checked = fullscreen;

	vsync = render->IsVsyncEnabled();
	cb_vsync->checked = vsync;

	const Int2& current_res = window->GetSize();
	const vector<Int2>& resolutions = render->GetAvailableResolutions();
	int selected_index = -1;
	for(uint index = 0; index < resolutions.size(); ++index)
	{
		if(current_res == resolutions[index])
		{
			selected_index = index;
			break;
		}
	}
	resolution_index = selected_index;
	ddl_resolution->selected_index = selected_index;
	ddl_resolution->is_open = false;

	volume = sound_mgr->GetSoundVolume();
	sl_volume->value = volume;

	gui->ShowDialog(this);
}

void Options::Close()
{
	Window* window = game_state->engine->GetWindow();
	Render* render = game_state->engine->GetRender();
	SoundManager* sound_mgr = game_state->engine->GetSoundManager();

	game_state->config->SetBool("fullscreen", window->IsFullscreen());
	game_state->config->SetBool("vsync", render->IsVsyncEnabled());
	game_state->config->SetInt2("resolution", window->GetSize());
	game_state->config->SetInt("volume", sound_mgr->GetSoundVolume());
	game_state->config->Save();

	gui->CloseDialog();
}

void Options::OnEvent(int id)
{
	Close();
}
