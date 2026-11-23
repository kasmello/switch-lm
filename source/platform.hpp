#pragma once

#include <cstddef>

// Platform module: the only place with Switch-specific (libnx) code.
// On the Switch it uses libnx; on a desktop build (make mac) it uses simple stand-ins.

void platform_init();
bool platform_running(); // false when the app should quit (Home menu close, + button)
void platform_exit();

// The system UI font as a TTF in memory (owned by the system; don't free it).
// Returns false where there is none (desktop build), so the caller should load its own font.
bool platform_font(void** data, std::size_t* size);

// True where text is typed through the system's on-screen keyboard (Switch), false where a
// physical keyboard types straight into the text box (desktop build).
bool platform_has_keyboard();

// Shows the on-screen keyboard prefilled with `text` and blocks until it closes.
// On OK, writes the result to `text` (NUL-terminated, at most textSize - 1 bytes) and returns true.
// On cancel, or where there is no on-screen keyboard, leaves `text` untouched and returns false.
bool platform_keyboard(char* text, std::size_t textSize);
