#include "platform.hpp"

#ifdef __SWITCH__
#include <switch.h>

namespace
{
PadState g_pad;
bool g_plReady = false;
} // namespace

void platform_init()
{
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
