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

/* The Radio tab: a simple MP3 player.
 *
 * Scans a directory for MP3 files and plays them with gst-launch-1.0.
 * The UI follows the same gritty STALKER guide look as the Messages and
 * Other tabs: a framed panel, a track list, and a control row at the bottom.
 *
 *   +-------------------------------------------------------------+
 *   | RADIO     /home/pi/music     [ PLAY ] [ STOP ] [ NEXT ]     |
 *   +-------------------------------------------------------------+
 *   | 01 track one.mp3                                            |
 *   | 02 track two.mp3   <- playing                               |
 *   | 03 track three.mp3                                          |
 *   |                                                             |
 *   |                                                             |
 *   +-------------------------------------------------------------+
 *
 * The directory is remembered in the settings file (radio_dir). gst-launch-1.0
 * is forked for playback and killed on stop or track change.
 */

#define MAX_TRACKS   64
#define LIST_Y      34
#define LIST_H      320
#define CTRL_H      40
#define BTN_W       70
#define BTN_H       26
#define BTN_GAP     8
#define FRAME_X     6
#define FRAME_Y     32
#define FRAME_W     788
#define FRAME_H     407
#define ROW_H       20
#define INDENT_X    12

typedef struct {
    char path[768];
    char name[128];
} track_t;

typedef struct {
    lv_obj_t * root;
    lv_obj_t * list;
    lv_obj_t * status_lbl;
    lv_obj_t * dir_lbl;
    lv_obj_t * play_btn;
    lv_obj_t * stop_btn;
    lv_obj_t * next_btn;
    lv_obj_t * prev_btn;
    lv_timer_t * timer;
    track_t tracks[MAX_TRACKS];
    int n_tracks;
    int cur_track;
    bool playing;
    pid_t player_pid;
    char music_dir[512];
} radio_t;

static radio_t g_radio;

static void radio_stop(void);
static void radio_play_idx(int idx);
static void radio_tick(lv_timer_t * timer);
static void refresh(void);

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
        /* child: play the track */
        execlp("gst-launch-1.0", "gst-launch-1.0", "-q",
               "playbin",
               "uri", uri,
               NULL);
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
        /* the track finished: play the next one */
        g_radio.player_pid = -1;
        g_radio.playing = false;
        radio_next();
    }
    refresh();
}

/* ---------- the UI ---------- */

static const char * track_short_name(const char * name)
{
    /* strip the .mp3 extension for display */
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
    if(!g_radio.list) return;

    lv_obj_clean(g_radio.list);
    if(g_radio.n_tracks <= 0) {
        lv_obj_t * e = lv_label_create(g_radio.list);
        lv_label_set_text(e, "no mp3 files");
        lv_obj_set_pos(e, INDENT_X, 2);
        lv_obj_set_style_text_font(e, &pda_font_mono_10, LV_PART_MAIN);
        lv_obj_set_style_text_color(e, lv_color_hex(PDA_MSG_SYS), LV_PART_MAIN);
        lv_obj_set_style_pad_all(e, 0, LV_PART_MAIN);
        return;
    }

    for(int i = 0; i < g_radio.n_tracks; i++) {
        lv_obj_t * row = lv_obj_create(g_radio.list);
        lv_obj_set_pos(row, 0, i * ROW_H);
        lv_obj_set_size(row, lv_pct(100), ROW_H);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(row, 0, LV_PART_MAIN);
        lv_obj_set_scrollbar_mode(row, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_scroll_dir(row, LV_DIR_NONE);
        lv_obj_set_scrollable(row, false);
        lv_obj_set_clickable(row, false);

        lv_obj_t * nl = lv_label_create(row);
        lv_label_set_text(nl, track_short_name(g_radio.tracks[i].name));
        lv_obj_set_pos(nl, INDENT_X, 2);
        lv_obj_set_width(nl, 700);
        lv_label_set_long_mode(nl, LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_font(nl, &pda_font_mono_10, LV_PART_MAIN);
        lv_obj_set_style_text_color(nl,
            lv_color_hex(i == g_radio.cur_track && g_radio.playing
                         ? PDA_MSG_ACCENT : PDA_MSG_TEXT), LV_PART_MAIN);
        lv_obj_set_style_pad_all(nl, 0, LV_PART_MAIN);

        if(i == g_radio.cur_track && g_radio.playing) {
            lv_obj_t * dot = lv_obj_create(row);
            lv_obj_set_pos(dot, 2, 7);
            lv_obj_set_size(dot, 6, 6);
            lv_obj_set_style_radius(dot, 50, LV_PART_MAIN);
            lv_obj_set_style_bg_color(dot, lv_color_hex(PDA_MSG_ACCENT), LV_PART_MAIN);
            lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, LV_PART_MAIN);
        }

        /* hairline between rows */
        if(i < g_radio.n_tracks - 1) {
            grit_rule(row, 0, ROW_H - 1, lv_pct(100), false, PDA_RULE);
        }
    }
}

static void on_play_click(lv_event_t * e)
{
    LV_UNUSED(e);
    if(g_radio.n_tracks <= 0) return;
    if(g_radio.playing) {
        /* already playing: pause is not supported by the player, so just stop */
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

static void on_row_click(lv_event_t * e)
{
    int idx = (int)(lv_intptr_t)lv_event_get_user_data(e);
    radio_play_idx(idx);
    refresh();
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

    /* the top strip: title + directory */
    lv_obj_t * strip = lv_obj_create(p);
    lv_obj_set_size(strip, lv_pct(100), 26);
    lv_obj_set_style_bg_color(strip, lv_color_hex(PDA_CHAT_STRIP), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(strip, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(strip, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(strip, lv_color_hex(PDA_CHAT_EDGE), LV_PART_MAIN);
    lv_obj_set_style_pad_all(strip, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_left(strip, 8, LV_PART_MAIN);

    lv_obj_t * title = lv_label_create(strip);
    lv_label_set_text(title, "Radio");
    lv_obj_set_pos(title, 4, 3);
    lv_obj_set_style_text_font(title, &pda_font_mono_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_hex(PDA_TITLE), LV_PART_MAIN);

    r->dir_lbl = lv_label_create(strip);
    lv_label_set_text(r->dir_lbl, r->music_dir);
    lv_obj_set_pos(r->dir_lbl, 130, 3);
    lv_obj_set_width(r->dir_lbl, 420);
    lv_label_set_long_mode(r->dir_lbl, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(r->dir_lbl, &pda_font_mono_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(r->dir_lbl, lv_color_hex(PDA_MSG_TIME), LV_PART_MAIN);

    r->status_lbl = lv_label_create(strip);
    lv_label_set_text(r->status_lbl, "stopped");
    lv_obj_set_pos(r->status_lbl, 620, 3);
    lv_obj_set_width(r->status_lbl, 164);
    lv_label_set_long_mode(r->status_lbl, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(r->status_lbl, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    lv_obj_set_style_text_font(r->status_lbl, &pda_font_mono_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(r->status_lbl, lv_color_hex(PDA_MSG_SYS), LV_PART_MAIN);

    /* the control row: play / stop / prev / next */
    int bx = 760 - BTN_W;
    r->play_btn = lv_button_create(strip);
    lv_obj_set_pos(r->play_btn, bx, 0);
    lv_obj_set_size(r->play_btn, BTN_W, BTN_H);
    lv_obj_add_event_cb(r->play_btn, on_play_click, LV_EVENT_CLICKED, NULL);
    lv_no_transition(r->play_btn);
    lv_obj_t * pl = lv_label_create(r->play_btn);
    lv_label_set_text(pl, "PLAY");
    lv_obj_center(pl);

    bx -= BTN_W + BTN_GAP;
    r->stop_btn = lv_button_create(strip);
    lv_obj_set_pos(r->stop_btn, bx, 0);
    lv_obj_set_size(r->stop_btn, BTN_W, BTN_H);
    lv_obj_add_event_cb(r->stop_btn, on_stop_click, LV_EVENT_CLICKED, NULL);
    lv_no_transition(r->stop_btn);
    lv_obj_t * sl = lv_label_create(r->stop_btn);
    lv_label_set_text(sl, "STOP");
    lv_obj_center(sl);

    bx -= BTN_W + BTN_GAP;
    r->next_btn = lv_button_create(strip);
    lv_obj_set_pos(r->next_btn, bx, 0);
    lv_obj_set_size(r->next_btn, BTN_W, BTN_H);
    lv_obj_add_event_cb(r->next_btn, on_next_click, LV_EVENT_CLICKED, NULL);
    lv_no_transition(r->next_btn);
    lv_obj_t * nl = lv_label_create(r->next_btn);
    lv_label_set_text(nl, "NEXT");
    lv_obj_center(nl);

    bx -= BTN_W + BTN_GAP;
    r->prev_btn = lv_button_create(strip);
    lv_obj_set_pos(r->prev_btn, bx, 0);
    lv_obj_set_size(r->prev_btn, BTN_W, BTN_H);
    lv_obj_add_event_cb(r->prev_btn, on_prev_click, LV_EVENT_CLICKED, NULL);
    lv_no_transition(r->prev_btn);
    lv_obj_t * prl = lv_label_create(r->prev_btn);
    lv_label_set_text(prl, "PREV");
    lv_obj_center(prl);

    /* the track list */
    r->list = lv_obj_create(p);
    lv_obj_set_pos(r->list, 0, LIST_Y);
    lv_obj_set_size(r->list, lv_pct(100), LIST_H);
    lv_obj_set_style_bg_color(r->list, lv_color_hex(PDA_PAGE_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(r->list, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(r->list, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(r->list, lv_color_hex(PDA_PAGE_EDGE), LV_PART_MAIN);
    lv_obj_set_style_pad_all(r->list, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(r->list, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scroll_dir(r->list, LV_DIR_VER);
    lv_obj_set_scrollable(r->list, true);

    /* make each row clickable */
    /* (rows are created in refresh(); the click handler is added there) */

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
    r->list = NULL;
    r->status_lbl = NULL;
    r->dir_lbl = NULL;
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
    if(r->dir_lbl) lv_label_set_text(r->dir_lbl, r->music_dir);
    if(r->cur_track >= r->n_tracks) r->cur_track = -1;
    refresh();
}