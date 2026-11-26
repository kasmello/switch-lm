#pragma once

// GPU module: deko3d compute on the Switch. SDL owns the screen, so this never touches the display.
// On a desktop build (make mac) there is no GPU backend: every function returns false.

bool gpu_init(); // create the deko3d device + compute queue; false on failure (error is logged)
void gpu_exit();

// Runs a tiny compute shader (shaders/selftest.glsl) and checks its output. Logs the result.
bool gpu_selftest();
