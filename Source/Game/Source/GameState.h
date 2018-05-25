#pragma once

class GameState
{
public:
	enum ChangeState
	{
		NONE,
		NEW_GAME,
		CONTINUE,
		EXIT_TO_MENU,
		QUIT
	};

	GameState() : change_state(NONE), paused(false) {}

	bool IsPaused() { return paused; }
	ChangeState GetChangeState()
	{
		ChangeState new_change_state = change_state;
		change_state = NONE;
		return new_change_state;
	}

	void SetChangeState(ChangeState new_change_state)
	{
		assert(new_change_state != NONE);
		change_state = new_change_state;
	}
	void SetPaused(bool paused) { this->paused = paused; }

private:
	ChangeState change_state;
	bool paused;
};

namespace global
{
	extern GameState state;
}