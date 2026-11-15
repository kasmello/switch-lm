#include "ui.hpp"

#include "platform.hpp"

#include <SDL2/SDL.h>
#include <cstdio>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"

namespace
{
constexpr float kFontSize = 24.0f; // base text size in pixels, before scaling
// Font used when there's no system font (desktop build). make mac-run runs from the repo root.
constexpr const char* kDesktopFont = "third_party/imgui/misc/fonts/Roboto-Medium.ttf";

SDL_Window* g_window = nullptr;
SDL_Renderer* g_renderer = nullptr;
bool g_imgui = false;
bool g_showDemo = false;

void load_font(ImGuiIO& io)
{
    void* data = nullptr;
    std::size_t size = 0;
    if (platform_font(&data, &size))
    {
        ImFontConfig cfg;
        cfg.FontDataOwnedByAtlas = false; // the system owns this memory
        io.Fonts->AddFontFromMemoryTTF(data, (int)size, kFontSize, &cfg);
        return;
    }
    if (std::FILE* f = std::fopen(kDesktopFont, "rb"))
    {
        std::fclose(f);
        io.Fonts->AddFontFromFileTTF(kDesktopFont, kFontSize);
        return;
    }
    SDL_Log("font not found (%s); using ImGui's built-in font", kDesktopFont);
    ImFontConfig cfg;
    cfg.SizePixels = kFontSize;
    io.Fonts->AddFontDefault(&cfg);
}
} // namespace

bool ui_init()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0)
    {
        SDL_Log("SDL_Init: %s", SDL_GetError());
        return false;
    }

#ifdef __SWITCH__
    // Full-screen 1080p; the Switch scales it down in handheld mode.
    const int winW = 1920, winH = 1080;
    const Uint32 winFlags = 0;
    const float scale = 1.5f; // UI sizes are designed for 720p; 1080 / 720 = 1.5
#else
    // Desktop: a resizable window that fits on a laptop screen.
    const int winW = 1280, winH = 720;
    const Uint32 winFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
    const float scale = ImGui_ImplSDL2_GetContentScaleForDisplay(0);
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

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    g_imgui = true;
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
    io.IniFilename = nullptr; // don't write imgui.ini (window positions) to disk

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(scale * 1.5f); // roomier than ImGui's defaults, for touch and controllers
    style.FontScaleDpi = scale;
    load_font(io);

    ImGui_ImplSDL2_InitForSDLRenderer(g_window, g_renderer);
    ImGui_ImplSDLRenderer2_Init(g_renderer);
    return true;
}

void ui_exit()
{
    if (g_imgui)
    {
        ImGui_ImplSDLRenderer2_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
        g_imgui = false;
    }
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
        ImGui_ImplSDL2_ProcessEvent(&e);
        if (e.type == SDL_QUIT)
            return false;
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
            return false;
    }
    return true;
}

void ui_present()
{
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // Placeholder screen: one full-screen window. The chat UI will replace this.
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::Begin("switch-lm", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::Text("switch-lm");
    ImGui::TextWrapped("Dear ImGui %s is running. The chat UI goes here.", ImGui::GetVersion());
    ImGui::Checkbox("Show ImGui demo window", &g_showDemo);
    ImGui::End();

    if (g_showDemo)
        ImGui::ShowDemoWindow(&g_showDemo); // every widget, with the code behind it

    ImGui::Render();
    ImGuiIO& io = ImGui::GetIO();
    SDL_RenderSetScale(g_renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
    SDL_SetRenderDrawColor(g_renderer, 20, 20, 28, 255);
    SDL_RenderClear(g_renderer);
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), g_renderer);
    SDL_RenderPresent(g_renderer);
}
