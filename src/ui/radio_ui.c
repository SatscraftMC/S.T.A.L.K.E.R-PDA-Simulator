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
#include <math.h>

/* The Radio tab: a simple MP3 player styled like the STALKER music player.
 *
 *   +-------------------------------------------------------------+
 *   |                      Music Player                           |
 *   +-------------------------------------------------------------+
 *   |  Shuffle Off             04:18            Loop Off          |
 *   |  Playlist: OST                                           ...|
 *   |  Playing: cheeki breeki but a bit classy                    |
 *   +-------------------------------------------------------------+
 *   | [≡] [↻] [↺]   [−] [+]   [◄◄] [■] [▶] [►►]                   |
 *   +-------------------------------------------------------------+
 *
 * Plays via gst-launch-1.0 (fork/exec + kill). Directory remembered in
 * prefs (radio_dir).
 *
 * The active elements (shuffle/loop/time/playing) are drawn in cyan, like
 * Image 1. The control buttons are outline-only parallelograms with
 * canvas-drawn icons, matching the STALKER guide look of the Other and
 * Messages tabs.
 */

#define MAX_TRACKS   64
#define PANEL_X      190
#define PANEL_Y      50
#define PANEL_W      400
#define PANEL_H      100
#define BTN_W        70
#define BTN_H        28
#define BTN_GAP      8
#define GROUP_GAP    12
#define BTN_SLANT    11

/* the cyan used for the active text, like Image 1 */
#define RADIO_CYAN   0x00B0D8

/* one canvas buffer per button, holding the parallelogram outline + icon */
static uint32_t g_icon_bufs[9][BTN_W * BTN_H];

typedef struct {
    char path[768];
    char name[128];
} track_t;

typedef struct text_canvas_t text_canvas_t;

typedef struct {
    lv_obj_t * root;
    lv_obj_t * panel;          /* central info panel */
    text_canvas_t * title_canvas;   /* "Music Player" title canvas */
    text_canvas_t * shuffle_canvas; /* Shuffle On/Off text canvas */
    text_canvas_t * time_canvas;    /* 04:18 text canvas */
    text_canvas_t * loop_canvas;    /* Loop On/Off text canvas */
    text_canvas_t * playlist_canvas;/* Playlist: OST text canvas */
    text_canvas_t * playing_canvas; /* Playing: ... text canvas */
    lv_obj_t * btn_canvases[9];
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
static const char * track_short_name(const char * name);
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

/* ---------- canvas-backed text ---------- */

/* a canvas that owns its pixel buffer, freed when the object is deleted */
struct text_canvas_t {
    lv_obj_t * canvas;
    uint32_t * buf;
    int w, h;
};

static void text_canvas_gone(lv_event_t * e)
{
    lv_obj_t * c = lv_event_get_current_target(e);
    text_canvas_t * t = (text_canvas_t *)lv_obj_get_user_data(c);
    if(t && t->buf) free(t->buf);
    free(t);
}

/* draw text into an ARGB8888 canvas buffer, centred vertically */
static text_canvas_t * text_canvas_create(lv_obj_t * parent, int x, int y,
                                          int w, int h,
                                          const char * text,
                                          const lv_font_t * font,
                                          uint32_t color)
{
    text_canvas_t * t = malloc(sizeof(text_canvas_t));
    if(!t) return NULL;
    memset(t, 0, sizeof(*t));

    t->buf = malloc((size_t)w * (size_t)h * sizeof(uint32_t));
    if(!t->buf) { free(t); return NULL; }
    memset(t->buf, 0, (size_t)w * (size_t)h * sizeof(uint32_t));
    t->w = w;
    t->h = h;

    t->canvas = lv_canvas_create(parent);
    lv_canvas_set_buffer(t->canvas, t->buf, w, h, LV_COLOR_FORMAT_ARGB8888);
    lv_obj_set_pos(t->canvas, x, y);
    lv_obj_set_size(t->canvas, w, h);
    lv_obj_set_style_bg_opa(t->canvas, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(t->canvas, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(t->canvas, 0, LV_PART_MAIN);
    lv_obj_set_clickable(t->canvas, false);
    lv_obj_set_user_data(t->canvas, t);
    lv_obj_add_event_cb(t->canvas, text_canvas_gone, LV_EVENT_DELETE, NULL);

    lv_layer_t layer;
    lv_canvas_init_layer(t->canvas, &layer);

    lv_point_t tsz;
    lv_text_get_size(&tsz, text, font, 0, 0, LV_COORD_MAX, LV_TEXT_ALIGN_LEFT);
    int tx = (w - (int)tsz.x) / 2;
    int ty = (h - (int)tsz.y) / 2;

    lv_draw_label_dsc_t dsc;
    lv_draw_label_dsc_init(&dsc);
    dsc.text = text;
    dsc.font = font;
    dsc.color = lv_color_hex(color);
    dsc.opa = LV_OPA_COVER;
    lv_area_t area = { tx, ty, tx + (int)tsz.x - 1, ty + (int)tsz.y - 1 };
    lv_draw_label(&layer, &dsc, &area);
    lv_canvas_finish_layer(t->canvas, &layer);

    return t;
}

static void text_canvas_set_text(text_canvas_t * t, const char * text,
                                 const lv_font_t * font, uint32_t color)
{
    if(!t || !t->canvas) return;
    memset(t->buf, 0, (size_t)t->w * (size_t)t->h * sizeof(uint32_t));

    lv_layer_t layer;
    lv_canvas_init_layer(t->canvas, &layer);

    lv_point_t tsz;
    lv_text_get_size(&tsz, text, font, 0, 0, LV_COORD_MAX, LV_TEXT_ALIGN_LEFT);
    int tx = (t->w - (int)tsz.x) / 2;
    int ty = (t->h - (int)tsz.y) / 2;

    lv_draw_label_dsc_t dsc;
    lv_draw_label_dsc_init(&dsc);
    dsc.text = text;
    dsc.font = font;
    dsc.color = lv_color_hex(color);
    dsc.opa = LV_OPA_COVER;
    lv_area_t area = { tx, ty, tx + (int)tsz.x - 1, ty + (int)tsz.y - 1 };
    lv_draw_label(&layer, &dsc, &area);
    lv_canvas_finish_layer(t->canvas, &layer);
}

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

/* ---------- parallelogram outline + icon drawing ---------- */

/* draw a parallelogram outline into an ARGB8888 buffer, slanted like the
   STALKER tab bar buttons. Color is 0xRRGGBB; alpha is set to 0xFF. */
static void draw_parallelogram_buf(uint32_t * buf, int w, int h, int slant,
                                   uint32_t color)
{
    color = 0xFF000000u | (color & 0xFFFFFFu);
    for(int y = 0; y < h; y++) {
        int shift = slant * (h - 1 - y) / (h - 1);
        int x1 = shift;
        int x2 = w - 1 + shift;
        if(y == 0 || y == h - 1) {
            for(int x = x1; x <= x2; x++) buf[y * w + x] = color;
        }
        else {
            buf[y * w + x1] = color;
            buf[y * w + x2] = color;
        }
    }
}

/* draw a filled rectangle into an ARGB8888 buffer */
static void fill_rect_buf(uint32_t * buf, int w, int h, int x0, int y0,
                          int x1, int y1, uint32_t color)
{
    color = 0xFF000000u | (color & 0xFFFFFFu);
    for(int y = y0; y <= y1 && y < h; y++) {
        for(int x = x0; x <= x1 && x < w; x++) {
            if(x >= 0 && y >= 0) buf[y * w + x] = color;
        }
    }
}

/* ---------- icon drawing ---------- */

/* hamburger menu: three horizontal bars */
static void draw_menu_icon(uint32_t * buf, int w, int h, uint32_t color)
{
    int cx = w / 2;
    int bar_w = 14;
    int bar_h = 2;
    int gap = 4;
    int y0 = h / 2 - bar_h * 3 - gap * 2;
    for(int b = 0; b < 3; b++) {
        fill_rect_buf(buf, w, h, cx - bar_w / 2, y0 + b * (bar_h + gap),
                      cx + bar_w / 2, y0 + b * (bar_h + gap) + bar_h - 1, color);
    }
}

/* repeat: circular arrow going clockwise */
static void draw_repeat_icon(uint32_t * buf, int w, int h, uint32_t color)
{
    int cx = w / 2, cy = h / 2;
    int r = 7;
    for(int a = 200; a <= 340; a++) {
        double rad = a * M_PI / 180.0;
        int x = cx + (int)(r * cos(rad) + 0.5);
        int y = cy + (int)(r * sin(rad) + 0.5);
        if(x >= 0 && x < w && y >= 0 && y < h) buf[y * w + x] = color;
    }
    int ax = cx + (int)(r * cos(340.0 * M_PI / 180.0) + 0.5);
    int ay = cy + (int)(r * sin(340.0 * M_PI / 180.0) + 0.5);
    fill_rect_buf(buf, w, h, ax - 1, ay - 1, ax + 1, ay + 1, color);
    fill_rect_buf(buf, w, h, cx - 1, cy - r - 1, cx + 1, cy - r + 1, color);
}

/* shuffle: two crossing arrows forming an X */
static void draw_shuffle_icon(uint32_t * buf, int w, int h, uint32_t color)
{
    int cx = w / 2, cy = h / 2;
    int r = 8;
    for(int d = -r; d <= r; d++) {
        int x = cx + d, y = cy + d;
        if(x >= 0 && x < w && y >= 0 && y < h) buf[y * w + x] = color;
    }
    for(int d = -r; d <= r; d++) {
        int x = cx + d, y = cy - d;
        if(x >= 0 && x < w && y >= 0 && y < h) buf[y * w + x] = color;
    }
    fill_rect_buf(buf, w, h, cx + r - 1, cy + r - 1, cx + r + 1, cy + r + 1, color);
    fill_rect_buf(buf, w, h, cx - r - 1, cy - r - 1, cx - r + 1, cy - r + 1, color);
}

/* volume down: speaker with minus */
static void draw_vol_down_icon(uint32_t * buf, int w, int h, uint32_t color)
{
    int cx = w / 2, cy = h / 2;
    fill_rect_buf(buf, w, h, cx - 5, cy - 5, cx - 1, cy + 5, color);
    for(int y = cy - 4; y <= cy + 4; y++) {
        int xw = 4 - abs(y - cy);
        for(int x = cx; x <= cx + xw && x < w; x++) {
            if(x >= 0 && y >= 0 && y < h) buf[y * w + x] = color;
        }
    }
    fill_rect_buf(buf, w, h, cx + 2, cy - 1, cx + 6, cy + 1, color);
}

/* volume up: speaker with plus */
static void draw_vol_up_icon(uint32_t * buf, int w, int h, uint32_t color)
{
    int cx = w / 2, cy = h / 2;
    fill_rect_buf(buf, w, h, cx - 5, cy - 5, cx - 1, cy + 5, color);
    for(int y = cy - 4; y <= cy + 4; y++) {
        int xw = 4 - abs(y - cy);
        for(int x = cx; x <= cx + xw && x < w; x++) {
            if(x >= 0 && y >= 0 && y < h) buf[y * w + x] = color;
        }
    }
    fill_rect_buf(buf, w, h, cx + 3, cy - 1, cx + 6, cy + 1, color);
    fill_rect_buf(buf, w, h, cx + 4, cy - 2, cx + 5, cy + 2, color);
}

/* previous: double left triangle */
static void draw_prev_icon(uint32_t * buf, int w, int h, uint32_t color)
{
    int cx = w / 2, cy = h / 2;
    for(int g = 0; g < 2; g++) {
        int ox = cx - 6 + g * 5;
        for(int dy = -4; dy <= 4; dy++) {
            int dx = 4 - abs(dy);
            for(int x = ox - dx; x <= ox; x++) {
                if(x >= 0 && x < w && cy + dy >= 0 && cy + dy < h)
                    buf[(cy + dy) * w + x] = color;
            }
        }
    }
}

/* stop: filled square */
static void draw_stop_icon(uint32_t * buf, int w, int h, uint32_t color)
{
    int cx = w / 2, cy = h / 2;
    fill_rect_buf(buf, w, h, cx - 4, cy - 4, cx + 4, cy + 4, color);
}

/* play: filled triangle pointing right */
static void draw_play_icon(uint32_t * buf, int w, int h, uint32_t color)
{
    int cx = w / 2, cy = h / 2;
    for(int dy = -5; dy <= 5; dy++) {
        int dx = 5 - abs(dy);
        for(int x = cx; x <= cx + dx && x < w; x++) {
            if(x >= 0 && cy + dy >= 0 && cy + dy < h)
                buf[(cy + dy) * w + x] = color;
        }
    }
}

/* next: double right triangle */
static void draw_next_icon(uint32_t * buf, int w, int h, uint32_t color)
{
    int cx = w / 2, cy = h / 2;
    for(int g = 0; g < 2; g++) {
        int ox = cx + 6 - g * 5;
        for(int dy = -4; dy <= 4; dy++) {
            int dx = 4 - abs(dy);
            for(int x = ox; x <= ox + dx && x < w; x++) {
                if(x >= 0 && cy + dy >= 0 && cy + dy < h)
                    buf[(cy + dy) * w + x] = color;
            }
        }
    }
}

typedef void (*icon_fn)(uint32_t *, int, int, uint32_t);

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
    lv_obj_set_scrollbar_mode(p, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scrollable(p, false);
    r->root = p;

    /* the gritty surface behind everything (like the Other/Messages tabs) */
    grit_backdrop(p, 800, 480);

    /* central info panel (Music Player) - framed like the guide screens */
    r->panel = grit_frame(p, PANEL_X - 2, PANEL_Y - 2, PANEL_W + 4, PANEL_H + 4,
                          PDA_CHAT_EDGE);
    lv_obj_set_style_bg_color(r->panel, lv_color_hex(PDA_CHAT_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(r->panel, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_clickable(r->panel, false);

    /* the inner content area of the panel */
    lv_obj_t * inner = lv_obj_create(r->panel);
    lv_obj_set_pos(inner, 3, 3);
    lv_obj_set_size(inner, PANEL_W + 4 - 7, PANEL_H + 4 - 7);
    lv_obj_set_style_bg_opa(inner, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(inner, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(inner, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(inner, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scrollable(inner, false);
    lv_obj_set_clickable(inner, false);

    /* title strip above the panel - canvas drawn text */
    r->title_canvas = text_canvas_create(p,
        PANEL_X + PANEL_W / 2 - 60, PANEL_Y - 24, 120, 20,
        "Music Player", &pda_font_mono_14, PDA_TITLE);

    /* row 1: Shuffle Off  04:18  Loop Off - cyan text */
    r->shuffle_canvas = text_canvas_create(inner,
        10, 0, 130, 16,
        "Shuffle Off", &pda_font_mono_10, RADIO_CYAN);
    r->time_canvas = text_canvas_create(inner,
        PANEL_W / 2 - 40, 0, 80, 16,
        "00:00", &pda_font_mono_10, RADIO_CYAN);
    r->loop_canvas = text_canvas_create(inner,
        PANEL_W - 130, 0, 130, 16,
        "Loop Off", &pda_font_mono_10, RADIO_CYAN);

    /* hairline under row 1 */
    grit_rule(inner, 0, 20, lv_pct(100), false, PDA_RULE);

    /* row 2: Playlist: OST - dim text */
    r->playlist_canvas = text_canvas_create(inner,
        10, 24, PANEL_W - 20, 16,
        "Playlist: OST", &pda_font_mono_10, PDA_MSG_TIME);

    /* row 3: Playing: ... - dim or cyan text */
    r->playing_canvas = text_canvas_create(inner,
        10, 40, PANEL_W - 20, 16,
        "Playing: -", &pda_font_mono_10, PDA_MSG_TEXT);

    /* hairline at bottom of panel */
    grit_rule(inner, 0, PANEL_H - 4, lv_pct(100), false, PDA_RULE);

    /* control row: outline-only parallelogram buttons with canvas-drawn icons */
    int btn_y = PANEL_Y + PANEL_H + 16;
    int total_w = 9 * BTN_W + 8 * BTN_GAP + 2 * GROUP_GAP;
    int start_x = (800 - total_w) / 2;

    /* buttons: menu, shuffle, loop, vol-, vol+, prev, stop, play, next */
    static const icon_fn g_icons[9] = {
        draw_menu_icon,
        draw_repeat_icon,
        draw_shuffle_icon,
        draw_vol_down_icon,
        draw_vol_up_icon,
        draw_prev_icon,
        draw_stop_icon,
        draw_play_icon,
        draw_next_icon,
    };

    for(int i = 0; i < 9; i++) {
        int x = start_x + i * BTN_W;
        for(int j = 0; j < i; j++)
            x += (j == 2 || j == 5) ? GROUP_GAP : BTN_GAP;

        /* the button: transparent, clickable, no border */
        lv_obj_t * b = lv_button_create(p);
        lv_obj_set_pos(b, x, btn_y);
        lv_obj_set_size(b, BTN_W, BTN_H);
        lv_obj_set_style_bg_opa(b, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(b, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(b, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(b, 0, LV_PART_MAIN);
        lv_no_transition(b);
        if(group) lv_group_add_obj(group, b);

        /* the canvas: parallelogram outline + icon */
        uint32_t * buf = g_icon_bufs[i];
        memset(buf, 0, sizeof(uint32_t) * (size_t)BTN_W * (size_t)BTN_H);
        draw_parallelogram_buf(buf, BTN_W, BTN_H, BTN_SLANT, PDA_TAB_STROKE);
        g_icons[i](buf, BTN_W, BTN_H, PDA_TAB_TEXT);

        lv_obj_t * c = lv_canvas_create(b);
        lv_canvas_set_buffer(c, buf, BTN_W, BTN_H, LV_COLOR_FORMAT_ARGB8888);
        lv_obj_set_pos(c, 0, 0);
        lv_obj_set_size(c, BTN_W, BTN_H);
        lv_obj_set_style_bg_opa(c, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(c, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(c, 0, LV_PART_MAIN);
        lv_obj_set_clickable(c, false);
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
    r->title_canvas = NULL;
    r->shuffle_canvas = NULL;
    r->time_canvas = NULL;
    r->loop_canvas = NULL;
    r->playlist_canvas = NULL;
    r->playing_canvas = NULL;
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

/* ---------- the panel refresh: redraws the info text canvases ---------- */

static void refresh(void)
{
    if(!g_radio.panel) return;

    /* row 1: Shuffle Off  04:18  Loop Off */
    text_canvas_set_text(g_radio.shuffle_canvas,
        g_radio.shuffle ? "Shuffle On" : "Shuffle Off",
        &pda_font_mono_10, RADIO_CYAN);

    char tbuf[16];
    snprintf(tbuf, sizeof(tbuf), "%02d:%02d",
             g_radio.playing ? (g_radio.cur_track / 60) : 0,
             g_radio.playing ? (g_radio.cur_track % 60) : 0);
    text_canvas_set_text(g_radio.time_canvas,
        tbuf, &pda_font_mono_10, RADIO_CYAN);

    text_canvas_set_text(g_radio.loop_canvas,
        g_radio.loop ? "Loop On" : "Loop Off",
        &pda_font_mono_10, RADIO_CYAN);

    /* row 3: Playing: ... */
    if(g_radio.playing && g_radio.n_tracks > 0) {
        char pl[256];
        snprintf(pl, sizeof(pl), "Playing: %s",
                 track_short_name(g_radio.tracks[g_radio.cur_track].name));
        text_canvas_set_text(g_radio.playing_canvas,
            pl, &pda_font_mono_10, RADIO_CYAN);
    }
    else {
        text_canvas_set_text(g_radio.playing_canvas,
            "Playing: -", &pda_font_mono_10, PDA_MSG_TEXT);
    }
}
