#include "app.h"

#include "lvgl/lvgl.h"

#include <signal.h>
#include <unistd.h>

/* SDL installs its own SIGTERM/SIGINT handler that tears the video subsystem
   down from signal context - that segfaults on exit. Override it with a plain
   async-signal-safe exit so the service stops cleanly (systemd sends SIGTERM). */
static void on_term(int sig)
{
    (void)sig;
    _exit(0);
}

int main(void)
{
    lv_init();

    lv_display_t * disp = lv_sdl_window_create(800, 480);
    lv_sdl_window_set_title(disp, "PDA // Simulator");

    lv_sdl_mouse_create();
    lv_sdl_keyboard_create();
    lv_sdl_mousewheel_create();

    signal(SIGTERM, on_term);
    signal(SIGINT, on_term);

    pda_app_init();

    while(1) {
        uint32_t idle = lv_timer_handler();
        if(idle > 16) idle = 16;
        lv_sleep_ms(idle);
    }

    return 0;
}
