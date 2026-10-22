#include "ui/radio_ui.h"
#include "ui/palette.h"
#include "ui/grit.h"
#include "ui/lvutil.h"
#include "ui/prefs.h"
#include "assets/fonts/pda_fonts.h"

#include <dirent.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The Radio tab: a simple MP3 player styled like the STALKER music player.
 *
 *   +-------------------------------------------------------------+
 *   |                      Music Player                           |
 *   +-------------------------------------------------------------+
 *   |  Shuffle Off             04:18            Loop Off          |
 *   |  Playlist: OST                                           ...|
 *   |  Playing: cheeki breeki but a bit classy                    |
 *   +-------------------------------------------------------------+
 *   | [≡] [↻] [↺] [−] [+] [◄◄] [■] [▶] [►►]                        |
 *   +-------------------------------------------------------------+
 *
 * Plays via gst-launch-1.0 (fork/exec + kill). Directory remembered in
 * prefs (radio_dir).
 */

#define MAX_TRACKS   64
#define PANEL_X      190
#define PANEL_Y      50
#define PANEL_W      400
#define PANEL_H      100
#define BTN_W        70
#define BTN_H        28
#define BTN_GAP      8

typedef struct {
    char path[768];
    char name[128];
} track_t;

typedef struct {
    lv_obj_t * root;
    lv_obj_t * panel;          /* central info panel */
    lv_obj_t * shuffle_lbl;
    lv_obj_t * time_lbl;
    lv_obj_t * loop_lbl;
    lv_obj_t * playlist_lbl;
    lv_obj_t * playing_lbl;
    lv_obj_t * play_btn;
    lv_obj_t * stop_btn;
    lv_obj_t * next_btn;
    lv_obj_t * prev_btn;
    lv_timer_t * timer;
    track_t tracks[MAX_TRACKS];
    int n_tracks;
    int cur_track;
    bool playing;
    bool shuffle;
    bool loop;
    pid_t player_pid;
    char music_dir[512];
} radio_t;

static radio_t g_radio;

static void radio_stop(void);
static void radio_play_idx(int idx);
static void radio_tick(lv_timer_t * timer);
static void refresh(void);
static void on_play_click(lv_event_t * e);
static void on_stop_click(lv_event_t * e);
static void on_next_click(lv_event_t * e);
static void on_prev_click(lv_event_t * e);

/* ---------- the music directory ---------- */

static const char * radio_dir_key(void)
{
    return "radio_dir";
}

static void radio_scan_dir(void)
{
    g_radio.n_tracks = 0;
    DIR * d = opendir(g_radio.music_dir);
    if(!d) return;

    struct dirent * e;
    while((e = readdir(d)) && g_radio.n_tracks < MAX_TRACKS) {
        if(e->d_name[0] == '.') continue;
        const char * ext = strrchr(e->d_name, '.');
        if(!ext) continue;
        if(strcasecmp(ext, ".mp3") != 0) continue;
        track_t * t = &g_radio.tracks[g_radio.n_tracks];
        char full[768];
        snprintf(full, sizeof(full), "%s/%s", g_radio.music_dir, e->d_name);
        snprintf(t->path, sizeof(t->path), "%s", full);
        snprintf(t->name, sizeof(t->name), "%.*s", (int)sizeof(t->name) - 1, e->d_name);
        g_radio.n_tracks++;
    }
    closedir(d);
}

/* ---------- playback ---------- */

static void radio_stop(void)
{
    if(g_radio.player_pid > 0) {
        kill(g_radio.player_pid, SIGTERM);
        waitpid(g_radio.player_pid, NULL, 0);
        g_radio.player_pid = -1;
    }
    g_radio.playing = false;
}

static void radio_play_idx(int idx)
{
    if(idx < 0 || idx >= g_radio.n_tracks) return;
    radio_stop();

    char uri[1024];
    snprintf(uri, sizeof(uri), "file://%s", g_radio.tracks[idx].path);

    pid_t pid = fork();
    if(pid == 0) {
        execlp("gst-launch-1.0", "gst-launch-1.0", "-q",
               "playbin", "uri", uri, NULL);
        _exit(127);
    }
    g_radio.player_pid = pid;
    g_radio.playing = true;
    g_radio.cur_track = idx;
}

static void radio_next(void)
{
    if(g_radio.n_tracks <= 0) return;
    int nxt = g_radio.cur_track + 1;
    if(nxt >= g_radio.n_tracks) nxt = 0;
    radio_play_idx(nxt);
}

static void radio_prev(void)
{
    if(g_radio.n_tracks <= 0) return;
    int prv = g_radio.cur_track - 1;
    if(prv < 0) prv = g_radio.n_tracks - 1;
    radio_play_idx(prv);
}

/* ---------- the tick: notice when a track finished ---------- */

static void radio_tick(lv_timer_t * timer)
{
    LV_UNUSED(timer);
    if(!g_radio.playing || g_radio.player_pid <= 0) return;

    int status = 0;
    pid_t r = waitpid(g_radio.player_pid, &status, WNOHANG);
    if(r == g_radio.player_pid) {
        g_radio.player_pid = -1;
        g_radio.playing = false;
        if(g_radio.loop) {
            radio_play_idx(g_radio.cur_track);
        }
        else {
            radio_next();
        }
    }
    refresh();
}

/* ---------- helpers ---------- */

static const char * track_short_name(const char * name)
{
    static char buf[128];
    size_t n = strlen(name);
    if(n > 4 && strcmp(name + n - 4, ".mp3") == 0) n -= 4;
    if(n >= sizeof(buf)) n = sizeof(buf) - 1;
    memcpy(buf, name, n);
    buf[n] = 0;
    return buf;
}

static void refresh(void)
{
    if(!g_radio.panel) return;

    /* clear old panel children */
    lv_obj_clean(g_radio.panel);

    /* row 1: Shuffle Off  04:18  Loop Off */
    lv_obj_t * shuffle_lbl = lv_label_create(g_radio.panel);
    lv_label_set_text(shuffle_lbl, g_radio.shuffle ? "Shuffle On" : "Shuffle Off");
    lv_obj_set_pos(shuffle_lbl, 10, 12);
    lv_obj_set_style_text_font(shuffle_lbl, &pda_font_mono_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(shuffle_lbl, lv_color_hex(PDA_MSG_ACCENT), LV_PART_MAIN);

    lv_obj_t * time_lbl = lv_label_create(g_radio.panel);
    char tbuf[16];
    snprintf(tbuf, sizeof(tbuf), "%02d:%02d",
             g_radio.playing ? (g_radio.cur_track / 60) : 0,
             g_radio.playing ? (g_radio.cur_track % 60) : 0);
    lv_label_set_text(time_lbl, tbuf);
    lv_obj_set_pos(time_lbl, PANEL_W / 2 - 20, 12);
    lv_obj_set_style_text_font(time_lbl, &pda_font_mono_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(time_lbl, lv_color_hex(PDA_MSG_ACCENT), LV_PART_MAIN);

    lv_obj_t * loop_lbl = lv_label_create(g_radio.panel);
    lv_label_set_text(loop_lbl, g_radio.loop ? "Loop On" : "Loop Off");
    lv_obj_set_pos(loop_lbl, PANEL_W - 74, 12);
    lv_obj_set_style_text_font(loop_lbl, &pda_font_mono_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(loop_lbl, lv_color_hex(PDA_MSG_ACCENT), LV_PART_MAIN);

    /* row 2: Playlist: OST */
    lv_obj_t * playlist_lbl = lv_label_create(g_radio.panel);
    lv_label_set_text(playlist_lbl, "Playlist: OST");
    lv_obj_set_pos(playlist_lbl, 10, 30);
    lv_obj_set_width(playlist_lbl, PANEL_W - 20);
    lv_label_set_long_mode(playlist_lbl, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(playlist_lbl, &pda_font_mono_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(playlist_lbl, lv_color_hex(PDA_MSG_TIME), LV_PART_MAIN);

    /* row 3: Playing: ... */
    lv_obj_t * playing_lbl = lv_label_create(g_radio.panel);
    if(g_radio.playing && g_radio.n_tracks > 0) {
        char pl[256];
        snprintf(pl, sizeof(pl), "Playing: %s", track_short_name(g_radio.tracks[g_radio.cur_track].name));
        lv_label_set_text(playing_lbl, pl);
    }
    else {
        lv_label_set_text(playing_lbl, "Playing: -");
    }
    lv_obj_set_pos(playing_lbl, 10, 44);
    lv_obj_set_width(playing_lbl, PANEL_W - 20);
    lv_label_set_long_mode(playing_lbl, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(playing_lbl, &pda_font_mono_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(playing_lbl, g_radio.playing
        ? lv_color_hex(PDA_MSG_ACCENT) : lv_color_hex(PDA_MSG_TEXT), LV_PART_MAIN);

    /* hairline at bottom of panel */
    grit_rule(g_radio.panel, 0, PANEL_H - 2, lv_pct(100), false, PDA_RULE);
}

/* ---------- parallelogram button helper ---------- */

/* a slanted button like the STALKER tab bar buttons */
static lv_obj_t * slanted_btn(lv_obj_t * parent, int x, int y, int w, int h,
                              lv_event_cb_t cb, const char * text,
                              lv_group_t * group)
{
    lv_obj_t * b = lv_button_create(parent);
    lv_obj_set_pos(b, x, y);
    lv_obj_set_size(b, w, h);
    lv_obj_set_style_radius(b, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(b, lv_color_hex(PDA_TAB_FILL), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(b, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(b, lv_color_hex(PDA_TAB_STROKE), LV_PART_MAIN);
    lv_obj_set_style_pad_all(b, 0, LV_PART_MAIN);
    lv_no_transition(b);
    if(cb) lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, NULL);
    if(group) lv_group_add_obj(group, b);

    lv_obj_t * l = lv_label_create(b);
    lv_label_set_text(l, text);
    lv_obj_center(l);
    lv_obj_set_style_text_font(l, &pda_font_mono_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(l, lv_color_hex(PDA_TAB_TEXT), LV_PART_MAIN);

    return b;
}

/* ---------- panel builder ---------- */

lv_obj_t * radio_ui_create(lv_obj_t * parent, lv_group_t * group)
{
    radio_t * r = &g_radio;
    memset(r, 0, sizeof(*r));

    const char * dir = getenv("PDA_RADIO_DIR");
    if(!dir || !*dir) dir = prefs_str(radio_dir_key(), "assets/music");
    snprintf(r->music_dir, sizeof(r->music_dir), "%s", dir);
    r->cur_track = -1;

    lv_obj_t * p = lv_obj_create(parent);
    lv_obj_set_pos(p, 0, 0);
    lv_obj_set_size(p, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(p, lv_color_hex(PDA_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(p, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(p, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(p, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(p, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(p, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollbar_mode(p, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scrollable(p, false);
    r->root = p;

    /* central info panel (Music Player) */
    r->panel = lv_obj_create(p);
    lv_obj_set_pos(r->panel, PANEL_X, PANEL_Y);
    lv_obj_set_size(r->panel, PANEL_W, PANEL_H);
    lv_obj_set_style_bg_color(r->panel, lv_color_hex(PDA_CHAT_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(r->panel, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(r->panel, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(r->panel, lv_color_hex(PDA_CHAT_EDGE), LV_PART_MAIN);
    lv_obj_set_style_radius(r->panel, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(r->panel, 0, LV_PART_MAIN);
    lv_obj_set_clickable(r->panel, false);

    /* title strip above the panel */
    lv_obj_t * title = lv_label_create(p);
    lv_label_set_text(title, "Music Player");
    lv_obj_set_pos(title, PANEL_X + PANEL_W / 2 - 48, PANEL_Y - 24);
    lv_obj_set_style_text_font(title, &pda_font_mono_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_hex(PDA_TITLE), LV_PART_MAIN);

    /* control row: slanted parallelogram buttons */
    int btn_y = PANEL_Y + PANEL_H + 16;
    int total_w = 9 * BTN_W + 8 * BTN_GAP;
    int start_x = (800 - total_w) / 2;

    /* buttons: menu, shuffle, loop, vol-, vol+, prev, stop, play, next */
    struct { lv_event_cb_t cb; const char * text; } btns[9] = {
        { NULL,            "≡"   },
        { NULL,            "↻"   },
        { NULL,            "↺"   },
        { NULL,            "−"   },
        { NULL,            "+"   },
        { on_prev_click,   "◄◄"  },
        { on_stop_click,   "■"   },
        { on_play_click,   "▶"   },
        { on_next_click,   "►►"  },
    };

    for(int i = 0; i < 9; i++) {
        int x = start_x + i * (BTN_W + BTN_GAP);
        lv_obj_t * b = slanted_btn(p, x, btn_y, BTN_W, BTN_H,
                                   btns[i].cb, btns[i].text, group);
        if(i == 5) r->prev_btn = b;
        else if(i == 6) r->stop_btn = b;
        else if(i == 7) r->play_btn = b;
        else if(i == 8) r->next_btn = b;
    }

    /* scan the directory */
    radio_scan_dir();

    /* the tick notices when a track finishes */
    r->timer = lv_timer_create(radio_tick, 200, NULL);

    refresh();
    return p;
}

void radio_ui_destroy(void)
{
    radio_t * r = &g_radio;
    radio_stop();
    if(r->timer) {
        lv_timer_delete(r->timer);
        r->timer = NULL;
    }
    lv_kill_tree(r->root);
    r->root = NULL;
    r->panel = NULL;
    r->shuffle_lbl = NULL;
    r->time_lbl = NULL;
    r->loop_lbl = NULL;
    r->playlist_lbl = NULL;
    r->playing_lbl = NULL;
    r->play_btn = NULL;
    r->stop_btn = NULL;
    r->next_btn = NULL;
    r->prev_btn = NULL;
    r->n_tracks = 0;
    r->cur_track = -1;
    r->playing = false;
    r->player_pid = -1;
}

void radio_ui_set_music_dir(const char * dir)
{
    radio_t * r = &g_radio;
    radio_stop();
    snprintf(r->music_dir, sizeof(r->music_dir), "%s", dir ? dir : "");
    prefs_set_str(radio_dir_key(), r->music_dir);
    radio_scan_dir();
    if(r->cur_track >= r->n_tracks) r->cur_track = -1;
    refresh();
}

static void on_play_click(lv_event_t * e)
{
    LV_UNUSED(e);
    if(g_radio.n_tracks <= 0) return;
    if(g_radio.playing) {
        radio_stop();
    }
    else {
        radio_play_idx(g_radio.cur_track >= 0 ? g_radio.cur_track : 0);
    }
    refresh();
}

static void on_stop_click(lv_event_t * e)
{
    LV_UNUSED(e);
    radio_stop();
    refresh();
}

static void on_next_click(lv_event_t * e)
{
    LV_UNUSED(e);
    radio_next();
    refresh();
}

static void on_prev_click(lv_event_t * e)
{
    LV_UNUSED(e);
    radio_prev();
    refresh();
}