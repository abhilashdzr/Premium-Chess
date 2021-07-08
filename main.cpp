#include "Window.h"

#include <SDL.h>
#include <iostream>
using namespace std;

signed main(int argv, char **argc)
{
	const int FPS = 60;
	const int wait = 1000 / FPS;

	Window* windowFrame = new Window();
	windowFrame->init("CHESS", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, false);

	uint32_t Start = SDL_GetTicks(), elapsed;
	while (windowFrame->running())
	{
		Start = SDL_GetTicks();
		windowFrame->update();
		elapsed = SDL_GetTicks() - Start;
		if (wait > elapsed)
			SDL_Delay(wait - elapsed);
	}

	windowFrame->clean();
	delete windowFrame;
}