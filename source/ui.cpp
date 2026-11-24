#include "ui.hpp"
#include "llm.hpp"

#include "platform.hpp"

#include <SDL2/SDL.h>
#include <cfloat>
#include <string>
#include <cstdio>
#include <algorithm>
#include <cctype>
#include <cstring>

#include "imgui.h"
#include "imgui_internal.h" // ImGui::ClearActiveID
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
    ImFontConfig cfg;
    cfg.SizePixels = kFontSize;
    io.Fonts->AddFontDefault(&cfg);
}
} // namespace

bool ui_init()
{
    // Touchscreen: ImGui's SDL2 backend only reads mouse events, so SDL must turn touches into
    // mouse events. The Switch SDL2 port turns that off when it starts (it sets
    // SDL_TOUCH_MOUSE_EVENTS=0 at default priority); setting it here, before SDL_Init, wins.
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "1");

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

        // A lifted finger leaves the (fake) mouse where it was, so the last widget touched would
        // stay highlighted. Move the pointer off-screen after a touch ends. Only for real
        // touchscreens: a Mac trackpad also sends finger events, but as an indirect device.
        if (e.type == SDL_FINGERUP &&
            SDL_GetTouchDeviceType(e.tfinger.touchId) == SDL_TOUCH_DEVICE_DIRECT)
            ImGui::GetIO().AddMousePosEvent(-FLT_MAX, -FLT_MAX);

        if (e.type == SDL_QUIT)
            return false;
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
            return false;
    }
    return true;
}

void add_msg_with_cap(int& msgCount, int& maxMessages)
{
    if (msgCount < maxMessages)
    {
        msgCount++;
    }
    else
    {
        SDL_Log("Maximum number of messages reached. Cannot add more messages.");
    }
}

static bool is_blank(const char* s)
{
    for (; *s; ++s)
        if (!std::isspace(static_cast<unsigned char>(*s)))
            return false;
    return true;
}

void draw_send_box(char* last_buf, int& msgCount, int& maxMessages, std::string& last_output)
{
    ImGui::BeginDisabled(is_blank(last_buf));
    if (ImGui::Button("Send"))
    {
        add_msg_with_cap(msgCount, maxMessages);
        last_output = dummy_message(last_buf);
    }
    ImGui::EndDisabled();
}

void draw_msx_box(char* buf, std::string& output, size_t bufSize)
{
    const ImGuiStyle& style = ImGui::GetStyle();
    const float lineH = ImGui::GetTextLineHeight();
    const int minLines = 1, maxLines = 7;

    const float boxW = ImGui::GetContentRegionAvail().x;
    float wrapW = boxW - style.FramePadding.x * 2;

    ImVec2 text = ImGui::CalcTextSize(buf, nullptr, false, wrapW);
    float textH = text.y;
    const size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') // CalcTextSize doesn't count an empty last line
        textH += lineH;

    if (textH > lineH * maxLines)
        textH = ImGui::CalcTextSize(buf, nullptr, false, wrapW - style.ScrollbarSize).y;

    textH = std::clamp(textH, lineH * minLines, lineH * maxLines);
    const float boxH = textH + style.FramePadding.y * 2;
    ImGuiInputTextFlags flags = ImGuiInputTextFlags_WordWrap;
    if (!output.empty())
        flags |= ImGuiInputTextFlags_ReadOnly;
    ImGui::InputTextMultiline("##input", buf, bufSize, ImVec2(-FLT_MIN, boxH), flags);
    // On the Switch, tapping (or pressing A on) the box opens the system keyboard instead.
    if (ImGui::IsItemActivated() && platform_has_keyboard())
    {
        platform_keyboard(buf, bufSize); // blocks until the keyboard closes
        // Deactivate the box: an active InputText keeps its own copy of the text and would
        // overwrite the keyboard's result next frame.
        ImGui::ClearActiveID();
        // The touch release happened while the keyboard was up, so ImGui never saw it.
        ImGui::GetIO().AddMouseButtonEvent(0, false);
    }
    if (!output.empty())
    {
        ImGui::TextWrapped("%s", output.c_str());
    }
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
    ImGui::TextWrapped("Switch-LM 1.1.0\nPrototype SLM.\nDear ImGui %s", ImGui::GetVersion());
    {
        static int maxMessages = 10;
        static int msgCount = 1;
        static char text[10][500];
        static std::string output[10];
        for (int i = 0; i < msgCount; ++i)
        {
            ImGui::PushID(i);
            draw_msx_box(text[i], output[i], IM_COUNTOF(text[i]));
            ImGui::PopID();
        }
        draw_send_box(text[msgCount - 1], msgCount, maxMessages, output[msgCount - 1]);
        ImGui::Checkbox("Show ImGui demo window", &g_showDemo);
        ImGui::End();
    }

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
