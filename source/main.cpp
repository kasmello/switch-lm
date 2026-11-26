#include "gpu.hpp"
#include "platform.hpp"
#include "ui.hpp"

int main(int argc, char* argv[])
{
    platform_init();
    if (!ui_init())
    {
        ui_exit();
        platform_exit();
        return 1;
    }

    // GPU compute is optional for now: the app runs without it (and always does on desktop).
    if (gpu_init())
        gpu_selftest();

    while (platform_running() && ui_poll())
        ui_present();

    gpu_exit();
    ui_exit();
    platform_exit();
    return 0;
}
