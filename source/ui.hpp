#pragma once

// UI module: owns the window, rendering and input. Nothing else in the app touches SDL.

bool ui_init(); // create window + renderer; false on failure (error is logged)
void ui_exit();
bool ui_poll();    // handle window/input events; false if the user asked to quit
void ui_present(); // clear, draw the current frame, show it