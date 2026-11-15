// Compiles the two Dear ImGui backends we use (SDL2 input + SDL_Renderer drawing).
// They're included here instead of listed in the Makefile because third_party/imgui/backends/
// also contains backends for other platforms (Vulkan, DirectX, Metal...) that must not be built.
#include "imgui_impl_sdl2.cpp"
#include "imgui_impl_sdlrenderer2.cpp"
