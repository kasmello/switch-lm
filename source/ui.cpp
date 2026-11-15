#include "ui.hpp"

#include <SDL2/SDL.h>

namespace
{
// Drawing coordinates are always 1920x1080; SDL scales them to the real window size.
constexpr int kWidth = 1920;
constexpr int kHeight = 1080;

SDL_Window* g_window = nullptr;
SDL_Renderer* g_renderer = nullptr;
} // namespace

bool ui_init()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) != 0)
    {
        SDL_Log("SDL_Init: %s", SDL_GetError());
        return false;
    }

#ifdef __SWITCH__
    const int winW = kWidth, winH = kHeight;
    const Uint32 winFlags = 0;
#else // desktop: a smaller, resizable window that fits on a laptop screen
    const int winW = 1280, winH = 720;
    const Uint32 winFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
#endif

    g_window = SDL_CreateWindow("switch-lm", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, winW,
                                winH, winFlags);
    if (!g_window)
    {
        SDL_Log("SDL_CreateWindow: %s", SDL_GetError());
        return false;
    }

    g_renderer =
        SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!g_renderer)
    {
        SDL_Log("SDL_CreateRenderer: %s", SDL_GetError());
        return false;
    }
    SDL_RenderSetLogicalSize(g_renderer, kWidth, kHeight);
    return true;
}

void ui_exit()
{
    if (g_renderer)
        SDL_DestroyRenderer(g_renderer);
    if (g_window)
        SDL_DestroyWindow(g_window);
    g_renderer = nullptr;
    g_window = nullptr;
    SDL_Quit();
}

bool ui_poll()
{
    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
        if (e.type == SDL_QUIT)
            return false;
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
            return false;
    }
    return true;
}

void ui_present()
{
    SDL_SetRenderDrawColor(g_renderer, 20, 20, 28, 255);
    SDL_RenderClear(g_renderer);
    SDL_RenderPresent(g_renderer);
}
