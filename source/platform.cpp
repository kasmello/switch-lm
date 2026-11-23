#include "platform.hpp"

#ifdef __SWITCH__
#include <switch.h>
#include <unistd.h> // close()

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

#endif
