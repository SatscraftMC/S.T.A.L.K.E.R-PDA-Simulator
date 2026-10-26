#include "app.h"
#include "ui/pda_shell.h"
#include "ui/radio_ui.h"

#include "lvgl/lvgl.h"

#include <SDL2/SDL.h>

/* Some SDL header/package combinations on this box are older than the
   runtime library and lack the modern event enum - use the documented
   numeric value (SDL 2.0.18+) and the legacy alias as a fallback. */
#ifndef SDL_EVENT_MOUSEWHEEL
#define SDL_EVENT_MOUSEWHEEL 0x403
#endif
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

/* SDL installs its own SIGTERM/SIGINT handler that tears the video subsystem
   down from signal context - that segfaults on exit. Override it with a plain
   async-signal-safe exit so the service stops cleanly (systemd sends SIGTERM). */
static volatile sig_atomic_t g_quit = 0;
/* set when the SDL window itself is gone: LVGL has already deleted the
   display (and on SDL_QUIT run lv_deinit), so the exit path must not touch
   LVGL objects any more - only the things that live outside it, the player
   child above all. */
static volatile sig_atomic_t g_sdl_gone = 0;

static void on_term(int sig)
{
    (void)sig;
    g_quit = 1;
}

/* SDL sees the wheel event before LVGL does; forward it to the map. */
static int SDLCALL on_sdl_event(void * userdata, SDL_Event * ev)
{
    LV_UNUSED(userdata);
    if(ev->type == SDL_QUIT) { g_sdl_gone = 1; g_quit = 1; }
    /* the WM close button: SDL_WINDOWEVENT_CLOSE first, SDL_QUIT follows
       when the last window is destroyed. LVGL handles both itself (it
       deletes the display, and with LV_SDL_DIRECT_EXIT would exit(0) from
       inside lv_timer_handler), so quit is flagged here, before that. */
    if(ev->type == SDL_WINDOWEVENT &&
       ev->window.event == SDL_WINDOWEVENT_CLOSE) { g_sdl_gone = 1; g_quit = 1; }
    if(ev->type == SDL_EVENT_MOUSEWHEEL || ev->type == SDL_MOUSEWHEEL) {
        int mx = 0, my = 0;
        SDL_GetMouseState(&mx, &my);
        pda_shell_wheel(ev->wheel.y, mx, my);
    }
    /* the right button is not a thing in LVGL's mouse: the map reads it
       here, to plan the route */
    if(ev->type == SDL_MOUSEBUTTONDOWN && ev->button.button == SDL_BUTTON_RIGHT) {
        pda_shell_right_click(ev->button.x, ev->button.y);
    }
    return 1;   /* keep the event for everyone else */
}

int main(void)
{
    lv_init();

    /* say which build this is: a process that was started before the last
       rebuild keeps the old code, and this is how you tell */
    fprintf(stderr, "[pda] %s\n", PDA_VERSION);

    lv_display_t * disp = lv_sdl_window_create(800, 480);
    /* the version is in the window title: a stale process is then obvious */
    lv_sdl_window_set_title(disp, "PDA // Simulator " PDA_VERSION);

    lv_sdl_mouse_create();
    lv_sdl_keyboard_create();
    /* the wheel is read directly from SDL (see on_sdl_event) so the map can
       zoom towards the cursor - LVGL 9 would route it to the focused object */

    signal(SIGTERM, on_term);
    signal(SIGINT, on_term);
    /* a relay that vanishes mid-write must fail the write, not the process:
       every send() on the new socket paths (and any SSL_write) then comes
       back as EPIPE instead of SIGPIPE killing us mid-tick. The carrier
       suites' "app alive after a dead peer" checks rest on this. */
    signal(SIGPIPE, SIG_IGN);

    SDL_AddEventWatch(on_sdl_event, NULL);

    pda_app_init();

    while(!g_quit) {
        uint32_t idle = lv_timer_handler();
        if(idle > 16) idle = 16;
        lv_sleep_ms(idle);
    }

    /* the music is killed first, and by a call that never goes near LVGL -
       the window was closed, LVGL may already be deinitialized by then */
    if(g_sdl_gone) radio_ui_stop_playback();
    else radio_ui_destroy();
    _exit(0);
    return 0;
}
