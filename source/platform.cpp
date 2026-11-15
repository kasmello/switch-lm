#include "platform.hpp"

#ifdef __SWITCH__
#include <switch.h>

namespace
{
PadState g_pad;
}

void platform_init()
{
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&g_pad);
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

#endif
