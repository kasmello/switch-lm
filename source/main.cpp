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

    while (platform_running() && ui_poll())
        ui_present();

    ui_exit();
    platform_exit();
    return 0;
}
