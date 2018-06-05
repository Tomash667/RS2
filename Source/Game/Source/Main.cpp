#include "GameCore.h"
#include "Game.h"
#include <Windows.h>

extern int _dummy;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	_dummy = 0;
	Game game;
	return game.Start(strcmp(lpCmdLine, "-qs") == 0);
}
