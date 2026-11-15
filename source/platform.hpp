#pragma once

// Platform module: the only place with Switch-specific (libnx) code.
// On the Switch it uses libnx; on a desktop build (make mac) it uses simple stand-ins.

void platform_init();
bool platform_running(); // false when the app should quit (Home menu close, + button)
void platform_exit();
