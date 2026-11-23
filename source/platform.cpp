#include "platform.hpp"

#ifdef __SWITCH__
#include <switch.h>
#include <unistd.h> // close()

#include <cstring>
#include <vector>

namespace
{
PadState g_pad;
bool g_plReady = false;
bool g_sockets = false;
int g_nxlinkSock = -1;
} // namespace

void platform_init()
{
    // printf / SDL_Log output: when launched with `make run` (nxlink -s), send stdout/stderr back
    // to the terminal on your computer. When launched from hbmenu normally, it goes nowhere.
    g_sockets = R_SUCCEEDED(socketInitializeDefault());
    if (g_sockets)
        g_nxlinkSock = nxlinkStdio();

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&g_pad);
    g_plReady = R_SUCCEEDED(plInitialize(PlServiceType_User)); // shared system fonts
}

bool platform_running()
{
    if (!appletMainLoop())
        return false;
    padUpdate(&g_pad);
    return !(padGetButtonsDown(&g_pad) & HidNpadButton_Plus); // + quits to hbmenu
}

void platform_exit()
{
    if (g_plReady)
        plExit();
    if (g_nxlinkSock >= 0)
        close(g_nxlinkSock);
    if (g_sockets)
        socketExit();
}

bool platform_font(void** data, std::size_t* size)
{
    PlFontData font;
    if (!g_plReady || R_FAILED(plGetSharedFontByType(&font, PlSharedFontType_Standard)))
        return false;
    *data = font.address;
    *size = font.size;
    return true;
}

bool platform_has_keyboard()
{
    return true;
}

bool platform_keyboard(char* text, std::size_t textSize)
{
    if (textSize < 2)
        return false;

    SwkbdConfig kbd;
    if (R_FAILED(swkbdCreate(&kbd, 0)))
        return false;
    swkbdConfigMakePresetDefault(&kbd);
    swkbdConfigSetInitialText(&kbd, text);
    swkbdConfigSetGuideText(&kbd, "Type a message");
    swkbdConfigSetStringLenMax(&kbd, textSize - 1);
    swkbdConfigSetTextDrawType(&kbd, SwkbdTextDrawType_Box); // multi-line box

    // Write into a scratch buffer so cancelling doesn't clobber the caller's text.
    std::vector<char> out(textSize, '\0');
    const bool ok = R_SUCCEEDED(swkbdShow(&kbd, out.data(), out.size())); // fails on cancel
    swkbdClose(&kbd);

    if (ok)
    {
        std::memcpy(text, out.data(), textSize);
        text[textSize - 1] = '\0';
    }
    return ok;
}

#else // desktop build: the window's close button / Esc (handled in ui_poll) quits

void platform_init()
{
}

bool platform_running()
{
    return true;
}

void platform_exit()
{
}

bool platform_font(void**, std::size_t*)
{
    return false;
}

bool platform_has_keyboard()
{
    return false;
}

bool platform_keyboard(char*, std::size_t)
{
    return false;
}

#endif
