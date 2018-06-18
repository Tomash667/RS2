#include "GameCore.h"
#include "Options.h"
#include "GameState.h"
#include <Engine.h>
#include <Window.h>
#include <Render.h>
#include <SoundManager.h>

Options::Options(GameState* state) : state(state)
{
	// TILE
	// [] Fullscreen
	// [] Vsync
	// [-----] Resolution
	// <----X> Volume
	//    [Ok]

	cb_fullscreen = new CheckBox;
	Add(cb_fullscreen);

	cb_vsync = new CheckBox;
	Add(cb_vsync);

	ddl_resolution = new DropDownList;
	const vector<Int2>& resolutions = state->engine->GetRender()->GetAvailableResolutions();
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
}

void Options::Draw()
{

}

void Options::Update(float dt)
{
	// handle alt enter here too
}

void Options::Show()
{
	Window* window = state->engine->GetWindow();
	Render* render = state->engine->GetRender();
	SoundManager* sound_mgr = state->engine->GetSoundManager();

	cb_fullscreen->checked = window->IsFullscreen();
	cb_vsync->checked = render->IsVsyncEnabled();
	const vector<Int2>& resolutions = render->GetAvailableResolutions();
	int selected_index = -1;
	Int2 current_res = window->GetSize();
	for(uint index = 0; index < resolutions.size(); ++index)
	{
		if(current_res == resolutions[index])
		{
			selected_index = index;
			break;
		}
	}
	ddl_resolution->selected_index = selected_index;
	sl_volume->value = sound_mgr->GetSoundVolume();
}

void Options::OnEvent(int id)
{

}
