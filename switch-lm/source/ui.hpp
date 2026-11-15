#pragma once

// UI module: owns the window, rendering and input. Nothing else in the app touches SDL.

bool ui_init(); // create window + renderer; false on failure (see SDL_GetError())
void ui_exit();
void ui_present(); // clear, draw the current frame, show it
