#include "Window.h"

#include <SDL.h>
#include <SDL_image.h>
#include <iostream>
using namespace std;

Board *Window::board;
SDL_Renderer* Window::rend = nullptr;                       //initializing the render window object

void Window::init(const char* title, int xpos, int ypos, bool fullscreen) {

    Uint32 rend_flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;

    if (SDL_Init(SDL_INIT_EVERYTHING) == 0)
    {
        window = SDL_CreateWindow(title, xpos, ypos,  800, 800, rend_flags);
        Window::rend = SDL_CreateRenderer(window, -1, rend_flags);

        isRunning = true;

        board = new Board();
        board->init();
    }
    else
        isRunning = false;
}

void Window::update()
{
    handleEvents();
    render();
}

void Window::handleEvents() {
    SDL_Event event;
    SDL_PollEvent(&event);

    switch (event.type) {
    case SDL_QUIT:
        isRunning = false;
        break;
    case SDL_KEYDOWN:
        handleKeyDown(event.key);
        break;
    case SDL_KEYUP:
        handleKeyUp(event.key);
        break;
    case SDL_MOUSEBUTTONDOWN:
        board->handleMouseButtonDown(event.button, board->getBoardState());
        break;
        // case SDL_MOUSEBUTTONUP:
        //     game.handleMouseUp(event.button);
        //     break;
    }
}


void Window::handleKeyDown(SDL_KeyboardEvent& key)
{
    switch (key.keysym.scancode)
    {
    case SDLK_r: // r_key
        board->reset();
        break;
    case SDLK_TAB: // tab_key
        board->togglePromotionOptions();
        break;
    default:
        break;
    }
}


void Window::handleKeyUp(SDL_KeyboardEvent& key)
{
    switch (key.keysym.scancode)
    {
    default:
        std::cout << "Scancode is:" << key.keysym.scancode << std::endl;
        break;
    }
}



void Window::clean()
{
    SDL_DestroyRenderer(Window::rend);
    SDL_DestroyWindow(window);  //error here
    SDL_Quit();
}



bool Window::running() { return isRunning; }



void Window::render() {
    // load the image into memory using SDL_image library function
    SDL_Surface* board_img = IMG_Load("resources/board.jpg");

    SDL_Texture* board_tex = SDL_CreateTextureFromSurface(Window::rend, board_img);
    SDL_FreeSurface(board_img);

    //clear the window
    SDL_RenderClear(Window::rend);
    //draw the window to the board
    SDL_RenderCopy(Window::rend, board_tex, NULL, NULL);
    board->render(board->getBoardState());
    SDL_RenderPresent(Window::rend);

    SDL_DestroyTexture(board_tex);
}