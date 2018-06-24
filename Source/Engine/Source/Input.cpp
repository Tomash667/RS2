#include "EngineCore.h"
#include "Input.h"

Input::Input() : mouse_dif(Int2::Zero), mouse_wheel(0)
{
	for(uint i = 0; i < (uint)Key::Max; ++i)
		keystate[i] = IS_UP;
}

void Input::Process(Key key, bool down)
{
	auto& k = keystate[(int)key];
	if(key != Key::PrintScreen)
	{
		if(down)
		{
			if(k <= IS_RELEASED)
				k = IS_PRESSED;
			keyrepeat[(int)key] = true;
		}
		else
		{
			if(k == IS_PRESSED)
				to_release.push_back(key);
			else if(k == IS_DOWN)
				k = IS_RELEASED;
		}
	}
	else
		k = IS_PRESSED;
}

void Input::Update()
{
	byte printscreen = keystate[(int)Key::PrintScreen];
	for(uint i = 0; i < (uint)Key::Max; ++i)
	{
		if(keystate[i] & 1)
			--keystate[i];
		keyrepeat[i] = false;
	}
	//for(uint i = 0; i < 5; ++i)
	//	doubleclk[i] = false;
	if(printscreen == IS_PRESSED)
		keystate[(int)Key::PrintScreen] = IS_RELEASED;
	for(Key k : to_release)
		keystate[(int)k] = IS_RELEASED;
	to_release.clear();
}

void Input::ReleaseKeys()
{
	for(uint i = 0; i < (uint)Key::Max; ++i)
	{
		if(keystate[i] & 0x2)
			keystate[i] = IS_RELEASED;
	}
	//for(uint i = 0; i < 5; ++i)
	//	doubleclk[i] = false;
	to_release.clear();
}
