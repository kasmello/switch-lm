#include "ui.hpp"

#include <SDL2/SDL.h>

namespace
{
constexpr int kWidth = 1280; // Switch renders at 1280x720 (scaled up when docked)
constexpr int kHeight = 720;

SDL_Window* g_window = nullptr;
SDL_Renderer* g_renderer = nullptr;
} // namespace

bool ui_init()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) != 0)
        return false;

    g_window = SDL_CreateWindow("switch-lm", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                kWidth, kHeight, 0);
    if (!g_window)
        return false;

    g_renderer =
        SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    return g_renderer != nullptr;
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

void ui_present()
{
    SDL_SetRenderDrawColor(g_renderer, 20, 20, 28, 255);
    SDL_RenderClear(g_renderer);
    SDL_RenderPresent(g_renderer);
}
