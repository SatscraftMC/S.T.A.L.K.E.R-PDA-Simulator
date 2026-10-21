#include "app.h"
#include "ui/pda_shell.h"

#include "lvgl/lvgl.h"

#include <SDL2/SDL.h>

/* Some SDL header/package combinations on this box are older than the
   runtime library and lack the modern event enum - use the documented
   numeric value (SDL 2.0.18+) and the legacy alias as a fallback. */
#ifndef SDL_EVENT_MOUSEWHEEL
#define SDL_EVENT_MOUSEWHEEL 0x403
#endif
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

/* SDL sees the wheel event before LVGL does; forward it to the map. */
static int SDLCALL on_sdl_event(void * userdata, SDL_Event * ev)
{
    LV_UNUSED(userdata);
    if(ev->type == SDL_EVENT_MOUSEWHEEL || ev->type == SDL_MOUSEWHEEL) {
        int mx = 0, my = 0;
        SDL_GetMouseState(&mx, &my);
        pda_shell_wheel(ev->wheel.y, mx, my);
    }
    return 1;   /* keep the event for everyone else */
}

int main(void)
{
    lv_init();

    lv_display_t * disp = lv_sdl_window_create(800, 480);
    lv_sdl_window_set_title(disp, "PDA // Simulator");

    lv_sdl_mouse_create();
    lv_sdl_keyboard_create();
    /* the wheel is read directly from SDL (see on_sdl_event) so the map can
       zoom towards the cursor - LVGL 9 would route it to the focused object */

    signal(SIGTERM, on_term);
    signal(SIGINT, on_term);

    SDL_AddEventWatch(on_sdl_event, NULL);

    pda_app_init();

    while(1) {
        uint32_t idle = lv_timer_handler();
        if(idle > 16) idle = 16;
        lv_sleep_ms(idle);
    }

    return 0;
}
