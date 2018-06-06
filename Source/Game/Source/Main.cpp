#include "GameCore.h"
#include "Game.h"
#include <Windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	Game game;
	game.start_fullscree = strcmp(lpCmdLine, "-full") == 0 || strcmp(lpCmdLine, "-fullhd") == 0;
	game.start_hd = strcmp(lpCmdLine, "-hd") == 0;
	return game.Start(strcmp(lpCmdLine, "-qs") == 0);
}
