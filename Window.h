#pragma once

#include "Board.h"

#include <SDL.h>
#include <vector> // char*

class Window
{
public:
	void clean();
	void render();
	bool running();


	void init(const char* title, int xpos, int ypos, bool fullscreen);
	void update();
	void handleEvents();
	void handleKeyDown(SDL_KeyboardEvent& key);
	void handleKeyUp(SDL_KeyboardEvent& key);
	static SDL_Renderer* rend;

	static Board *board;

private:
	bool isRunning;
	SDL_Window* window;
};
