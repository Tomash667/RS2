#pragma once

#include "Key.h"

class Input
{
private:
	enum InputState
	{
		IS_UP,			// 00
		IS_RELEASED,	// 01
		IS_DOWN,		// 10
		IS_PRESSED		// 11
	};

public:
	Input();
	void Process(Key key, bool down);
	void Update();
	void ReleaseKeys();

	bool Pressed(Key key) const { return keystate[(int)key] == IS_PRESSED; }
	bool PressedOnce(Key key)
	{
		if(Pressed(key))
		{
			keystate[(int)key] = IS_UP;
			return true;
		}
		return false;
	}
	bool Released(Key key) const { return keystate[(int)key] == IS_RELEASED; }
	bool Down(Key key) const { return keystate[(int)key] >= IS_DOWN; }
	bool DownRepeat(Key key) const { return Down(key) && keyrepeat[(int)key]; }
	bool Up(Key key) const { return keystate[(int)key] <= IS_RELEASED; }

	void SetMouseDif(const Int2& mouse_dif) { this->mouse_dif = mouse_dif; }
	void SetMouseWheel(int mouse_wheel) { this->mouse_wheel = mouse_wheel; }

	const Int2& GetMouseDif() { return mouse_dif; }
	int GetMouseWheel() { return mouse_wheel; }

private:
	vector<Key> to_release;
	byte keystate[(uint)Key::Max];
	bool keyrepeat[(uint)Key::Max];
	Int2 mouse_dif;
	int mouse_wheel;
};
