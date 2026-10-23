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
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#pragma GCC diagnostic ignored "-Wformat-truncation"

/* The Radio tab: a simple MP3 player styled like the STALKER music player.
 *
 *   +-------------------------------------------------------------+
 *   |                      Music Player                           |
 *   +-------------------------------------------------------------+
 *   |  Shuffle Off             04:18            Loop Off          |
 *   |  Playlist: OST                                           ...|
 *   |  Playing: cheeki breeki but a bit classy                    |
 *   +-------------------------------------------------------------+
 *   | [≣] [⟳] [⤨]  [−] [+]  [◀|] [■] [▶] [|▶]                     |
 *   +-------------------------------------------------------------+
 *
 * Plays via gst-launch-1.0 (fork/exec + kill). Directory remembered in
 * prefs (radio_dir).
 *
 * The active elements (shuffle/loop/time/playing) are drawn in cyan, like
 * Image 1. The control buttons are rounded dark plates (like the image:
 * slightly lighter fill, a faint 1px edge, light grey icons) in three
 * groups: playlist/repeat/shuffle, volume down/up, prev/stop/play/next.
 * Every plate and icon is canvas-drawn pixel by pixel.
 */

#define MAX_TRACKS   64
#define PANEL_X      190
#define PANEL_Y      50
#define PANEL_W      400
#define PANEL_H      100
#define BTN_W        58
#define BTN_H        30
#define BTN_GAP      5
#define GROUP_GAP    27
#define BTN_RADIUS   7

/* the cyan used for the active text, like Image 1 */
#define RADIO_CYAN   0x00B0D8

/* the button row, like Image 1: dark rounded plates with light grey icons */
#define RADIO_BTN_FILL  0x262628
#define RADIO_BTN_EDGE  0x4A4A4C
#define RADIO_BTN_ICON  0xADB0B5

/* one canvas buffer per button, holding the plate + icon */
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
    text_canvas_t * folder_canvas;  /* Folder: <name> text canvas */
    lv_obj_t * track_list;         /* scrollable list of tracks */
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
    char music_root[512];
    char music_dir[512];
    char folders[32][128];
    int n_folders;
    int folder_idx;
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
static void on_shuffle_click(lv_event_t * e);
static void on_loop_click(lv_event_t * e);
static void on_folder_click(lv_event_t * e);
static void on_track_click(lv_event_t * e);
static void radio_build_track_list(void);

/* ---------- the music directory ---------- */

static const char * radio_dir_key(void)
{
    return "radio_dir";
}

/* collect the subdirectories of music_root, sorted */
static void radio_list_folders(void)
{
    radio_t * r = &g_radio;
    r->n_folders = 0;
    r->folder_idx = -1;
    DIR * d = opendir(r->music_root);
    if(!d) return;
    struct dirent * e;
    char tmp[32][128];
    int n = 0;
    while((e = readdir(d)) && n < 32) {
        if(e->d_name[0] == '.') continue;
        char full[768];
        snprintf(full, sizeof(full), "%s/%s", r->music_root, e->d_name);
        struct stat st;
        if(stat(full, &st) != 0) continue;
        if(!S_ISDIR(st.st_mode)) continue;
        snprintf(tmp[n], sizeof(tmp[n]), "%s", e->d_name);
        n++;
    }
    closedir(d);
    /* sort */
    for(int i = 0; i < n; i++)
        for(int j = i + 1; j < n; j++)
            if(strcmp(tmp[i], tmp[j]) > 0) {
                char t[128]; memcpy(t, tmp[i], 128);
                memcpy(tmp[i], tmp[j], 128); memcpy(tmp[j], t, 128);
            }
    for(int i = 0; i < n; i++) {
        snprintf(r->folders[i], sizeof(r->folders[i]), "%s", tmp[i]);
        r->n_folders++;
    }
    /* figure out current folder index */
    const char * base = strrchr(r->music_dir, '/');
    base = base ? base + 1 : r->music_dir;
    for(int i = 0; i < r->n_folders; i++) {
        if(strcmp(r->folders[i], base) == 0) { r->folder_idx = i; break; }
    }
}

static const char * folder_basename(const char * path)
{
    static char buf[128];
    const char * b = strrchr(path, '/');
    b = b ? b + 1 : path;
    snprintf(buf, sizeof(buf), "%s", b);
    return buf;
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

    char abspath[4096];
    if(!realpath(g_radio.tracks[idx].path, abspath))
        snprintf(abspath, sizeof(abspath), "%s", g_radio.tracks[idx].path);

    char uri[4112];
    snprintf(uri, sizeof(uri), "file://%s", abspath);

    char uriprop[4120];
    snprintf(uriprop, sizeof(uriprop), "uri=%s", uri);

    pid_t pid = fork();
    if(pid == 0) {
        execlp("gst-launch-1.0", "gst-launch-1.0", "-q",
               "playbin", uriprop, NULL);
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

/* ---------- rounded plate + icon drawing ---------- */

/* draw a filled rounded plate with a 1px edge into an ARGB8888 buffer.
   Colors are 0xRRGGBB; alpha is set to 0xFF (the ARGB gotcha). */
static void draw_plate_buf(uint32_t * buf, int w, int h, int radius,
                           uint32_t fill, uint32_t edge)
{
    for(int y = 0; y < h; y++) {
        for(int x = 0; x < w; x++) {
            double px = x + 0.5, py = y + 0.5;
            double qx = (px < radius) ? radius :
                        (px > w - radius ? w - radius : px);
            double qy = (py < radius) ? radius :
                        (py > h - radius ? h - radius : py);
            double d = sqrt((px - qx) * (px - qx) + (py - qy) * (py - qy));
            uint32_t c = (d > radius - 1.0) ? edge : fill;
            buf[y * w + x] = 0xFF000000u | (c & 0xFFFFFFu);
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

/* draw a line (Bresenham) into an ARGB8888 buffer */
static void draw_line_buf(uint32_t * buf, int w, int h,
                          int x0, int y0, int x1, int y1, uint32_t color)
{
    color = 0xFF000000u | (color & 0xFFFFFFu);
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for(;;) {
        if(x0 >= 0 && x0 < w && y0 >= 0 && y0 < h) buf[y0 * w + x0] = color;
        if(x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if(e2 >= dy) { err += dy; x0 += sx; }
        if(e2 <= dx) { err += dx; y0 += sy; }
    }
}

/* fill a triangle into an ARGB8888 buffer (point-in-triangle test) */
static void fill_triangle_buf(uint32_t * buf, int w, int h,
                              double x0, double y0, double x1, double y1,
                              double x2, double y2, uint32_t color)
{
    color = 0xFF000000u | (color & 0xFFFFFFu);
    int xmn = (int)floorf((float)fmin(x0, fmin(x1, x2)));
    int ymn = (int)floorf((float)fmin(y0, fmin(y1, y2)));
    int xmx = (int)ceilf((float)fmax(x0, fmax(x1, x2)));
    int ymx = (int)ceilf((float)fmax(y0, fmax(y1, y2)));
    double area2 = (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0);
    if(fabs(area2) < 1e-9) return;
    for(int y = ymn; y <= ymx && y < h; y++) {
        for(int x = xmn; x <= xmx && x < w; x++) {
            if(x < 0 || y < 0) continue;
            double px = x + 0.5, py = y + 0.5;
            double s = (x1 - x0) * (py - y0) - (y1 - y0) * (px - x0);
            double t = (x2 - x1) * (py - y1) - (y2 - y1) * (px - x1);
            double u = (x0 - x2) * (py - y2) - (y0 - y2) * (px - x2);
            if((s >= 0 && t >= 0 && u >= 0) || (s <= 0 && t <= 0 && u <= 0))
                buf[y * w + x] = color;
        }
    }
}

/* a solid triangular arrowhead at (tx,ty), pointing along the unit
   direction (dx,dy), with a small overshoot so it caps the stroke */
static void draw_arrowhead_buf(uint32_t * buf, int w, int h,
                               double tx, double ty, double dx, double dy,
                               uint32_t color)
{
    double n = sqrt(dx * dx + dy * dy);
    if(n < 1e-6) return;
    dx /= n; dy /= n;
    double px = -dy, py = dx;
    double txp = tx + dx * 2, typ = ty + dy * 2;
    double b1x = tx - dx * 5 + px * 3, b1y = ty - dy * 5 + py * 3;
    double b2x = tx - dx * 5 - px * 3, b2y = ty - dy * 5 - py * 3;
    fill_triangle_buf(buf, w, h, txp, typ, b1x, b1y, b2x, b2y, color);
}

/* ---------- icon drawing ---------- */

/* playlist: four lines with a bullet beside each */
static void draw_playlist_icon(uint32_t * buf, int w, int h, uint32_t color)
{
    int cx = w / 2, cy = h / 2;
    for(int r = 0; r < 4; r++) {
        int y = cy - 7 + r * 5;
        fill_rect_buf(buf, w, h, cx - 10, y, cx - 9, y + 1, color);
        fill_rect_buf(buf, w, h, cx - 6, y, cx + 9, y + 1, color);
    }
}

/* repeat: a clockwise loop of two thick arcs with solid arrowheads
   (top arc travels left->top->right, bottom arc right->bottom->left) */
static void draw_repeat_icon(uint32_t * buf, int w, int h, uint32_t color)
{
    int cx = w / 2, cy = h / 2;
    int ex = 0, ey = 0;
    for(int a = 195; a <= 335; a++) {
        double rad = a * M_PI / 180.0;
        for(int k = 0; k < 2; k++) {
            double rx = 9 - 1.5 * k, ry = 6.5 - 1.3 * k;
            int x = cx + (int)(rx * cos(rad) + 0.5);
            int y = cy + (int)(ry * sin(rad) + 0.5);
            if(x >= 0 && x < w && y >= 0 && y < h) buf[y * w + x] = color;
        }
        if(a == 335) { ex = cx + (int)(9 * cos(rad) + 0.5);
                       ey = cy + (int)(6.5 * sin(rad) + 0.5); }
    }
    double rad = 335.0 * M_PI / 180.0;
    draw_arrowhead_buf(buf, w, h, ex, ey, -9 * sin(rad), 6.5 * cos(rad), color);

    for(int a = 15; a <= 165; a++) {
        double rad = a * M_PI / 180.0;
        for(int k = 0; k < 2; k++) {
            double rx = 9 - 1.5 * k, ry = 6.5 - 1.3 * k;
            int x = cx + (int)(rx * cos(rad) + 0.5);
            int y = cy + (int)(ry * sin(rad) + 0.5);
            if(x >= 0 && x < w && y >= 0 && y < h) buf[y * w + x] = color;
        }
        if(a == 165) { ex = cx + (int)(9 * cos(rad) + 0.5);
                       ey = cy + (int)(6.5 * sin(rad) + 0.5); }
    }
    rad = 165.0 * M_PI / 180.0;
    draw_arrowhead_buf(buf, w, h, ex, ey, -9 * sin(rad), 6.5 * cos(rad), color);
}

/* shuffle: two thick crossing arrows with solid arrowheads on the right ends */
static void draw_shuffle_icon(uint32_t * buf, int w, int h, uint32_t color)
{
    int cx = w / 2, cy = h / 2;
    draw_line_buf(buf, w, h, cx - 10, cy + 6, cx + 7, cy - 7, color);
    draw_line_buf(buf, w, h, cx - 10, cy + 7, cx + 7, cy - 6, color);
    draw_line_buf(buf, w, h, cx - 10, cy - 6, cx + 7, cy + 7, color);
    draw_line_buf(buf, w, h, cx - 10, cy - 7, cx + 7, cy + 6, color);
    draw_arrowhead_buf(buf, w, h, cx + 7, cy - 7, 1.0, -1.0, color);
    draw_arrowhead_buf(buf, w, h, cx + 7, cy + 7, 1.0, 1.0, color);
}

/* speaker body: solid box on the left, horn flaring to the right */
static void draw_speaker_buf(uint32_t * buf, int w, int h,
                             int cx, int cy, uint32_t color)
{
    fill_rect_buf(buf, w, h, cx - 7, cy - 3, cx - 5, cy + 3, color);
    for(int x = cx - 4; x <= cx - 1; x++) {
        int flare = (cx - 1) - x;
        int top = cy - 3 - flare;
        int bot = cy + 3 + flare;
        fill_rect_buf(buf, w, h, x, top, x, bot, color);
    }
}

/* volume down: speaker with minus */
static void draw_vol_down_icon(uint32_t * buf, int w, int h, uint32_t color)
{
    int cx = w / 2, cy = h / 2;
    draw_speaker_buf(buf, w, h, cx, cy, color);
    fill_rect_buf(buf, w, h, cx + 2, cy - 1, cx + 7, cy + 1, color);
}

/* volume up: speaker with plus */
static void draw_vol_up_icon(uint32_t * buf, int w, int h, uint32_t color)
{
    int cx = w / 2, cy = h / 2;
    draw_speaker_buf(buf, w, h, cx, cy, color);
    fill_rect_buf(buf, w, h, cx + 2, cy - 1, cx + 7, cy + 1, color);
    fill_rect_buf(buf, w, h, cx + 4, cy - 3, cx + 5, cy + 3, color);
}

/* previous: filled left triangle with a bar on its right */
static void draw_prev_icon(uint32_t * buf, int w, int h, uint32_t color)
{
    int cx = w / 2, cy = h / 2;
    fill_rect_buf(buf, w, h, cx + 4, cy - 7, cx + 5, cy + 7, color);
    for(int dy = -6; dy <= 6; dy++) {
        int x0 = cx - 9 + abs(dy) * 10 / 6;
        for(int x = x0; x <= cx + 1; x++)
            if(x >= 0 && x < w) buf[(cy + dy) * w + x] = color;
    }
}

/* stop: filled square */
static void draw_stop_icon(uint32_t * buf, int w, int h, uint32_t color)
{
    int cx = w / 2, cy = h / 2;
    fill_rect_buf(buf, w, h, cx - 5, cy - 5, cx + 5, cy + 5, color);
}

/* play: filled triangle pointing right */
static void draw_play_icon(uint32_t * buf, int w, int h, uint32_t color)
{
    int cx = w / 2, cy = h / 2;
    for(int dy = -6; dy <= 6; dy++) {
        int x1 = cx - 6 + (6 - abs(dy)) * 10 / 6;
        for(int x = cx - 6; x <= x1 && x < w; x++)
            if(x >= 0 && cy + dy >= 0 && cy + dy < h)
                buf[(cy + dy) * w + x] = color;
    }
}

/* next: filled right triangle with a bar on its left */
static void draw_next_icon(uint32_t * buf, int w, int h, uint32_t color)
{
    int cx = w / 2, cy = h / 2;
    fill_rect_buf(buf, w, h, cx - 5, cy - 7, cx - 4, cy + 7, color);
    for(int dy = -6; dy <= 6; dy++) {
        int x1 = cx + 9 - abs(dy) * 10 / 6;
        for(int x = cx - 1; x <= x1 && x < w; x++)
            if(x >= 0 && cy + dy >= 0 && cy + dy < h)
                buf[(cy + dy) * w + x] = color;
    }
}

typedef void (*icon_fn)(uint32_t *, int, int, uint32_t);


/* ---------- track list (music manager) ---------- */

static void on_track_click(lv_event_t * e)
{
    int idx = (int)(size_t)lv_event_get_user_data(e);
    if(idx < 0 || idx >= g_radio.n_tracks) return;
    radio_play_idx(idx);
    refresh();
}

static void radio_build_track_list(void)
{
    radio_t * r = &g_radio;
    if(!r->track_list) return;
    lv_obj_clean(r->track_list);

    if(r->n_tracks <= 0) {
        lv_obj_t * lbl = lv_label_create(r->track_list);
        lv_label_set_text(lbl, "No tracks in this folder");
        lv_obj_set_style_text_color(lbl, lv_color_hex(PDA_MSG_TIME), LV_PART_MAIN);
        lv_obj_set_style_text_font(lbl, &pda_font_mono_10, LV_PART_MAIN);
        return;
    }

    for(int i = 0; i < r->n_tracks; i++) {
        lv_obj_t * b = lv_button_create(r->track_list);
        lv_obj_set_width(b, lv_pct(100));
        lv_obj_set_height(b, 26);
        lv_obj_set_style_bg_color(b, lv_color_hex(PDA_CHAT_BG), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(b, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(b, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(b, 3, LV_PART_MAIN);
        lv_obj_set_style_pad_all(b, 4, LV_PART_MAIN);
        lv_obj_add_event_cb(b, on_track_click, LV_EVENT_CLICKED,
                            (void *)(size_t)i);

        lv_obj_t * lbl = lv_label_create(b);
        lv_label_set_text(lbl, track_short_name(r->tracks[i].name));
        lv_obj_set_style_text_color(lbl, lv_color_hex(PDA_MSG_TEXT), LV_PART_MAIN);
        lv_obj_set_style_text_font(lbl, &pda_font_mono_10, LV_PART_MAIN);
        lv_obj_set_width(lbl, lv_pct(100));
    }
}

/* ---------- panel builder ---------- */

lv_obj_t * radio_ui_create(lv_obj_t * parent, lv_group_t * group)
{
    radio_t * r = &g_radio;
    memset(r, 0, sizeof(*r));

    const char * root = getenv("PDA_MUSIC_ROOT");
    if(!root || !*root) root = "music";
    snprintf(r->music_root, sizeof(r->music_root), "%s", root);

    const char * dir = getenv("PDA_RADIO_DIR");
    if(!dir || !*dir) dir = prefs_str(radio_dir_key(), "");
    if(dir && *dir) {
        snprintf(r->music_dir, sizeof(r->music_dir), "%s", dir);
    }
    else {
        radio_list_folders();
        if(r->n_folders > 0) {
            r->folder_idx = 0;
        const char * _mr = (const char *)(void *)r->music_root;
        const char * _fn = (const char *)(void *)r->folders[0];
        snprintf(r->music_dir, sizeof(r->music_dir), "%s/%s", _mr, _fn);
        }
        else {
            const char * _mr2 = (const char *)(void *)r->music_root;
            snprintf(r->music_dir, sizeof(r->music_dir), "%s", _mr2);
        }
    }
    radio_list_folders();
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

    /* row 2: Folder: <name> - dim text */
    r->folder_canvas = text_canvas_create(inner,
        10, 24, PANEL_W - 20, 16,
        "Folder: -", &pda_font_mono_10, PDA_MSG_TIME);

    /* row 3: Playing: ... - dim or cyan text */
    r->playing_canvas = text_canvas_create(inner,
        10, 40, PANEL_W - 20, 16,
        "Playing: -", &pda_font_mono_10, PDA_MSG_TEXT);

    /* hairline at bottom of panel */
    grit_rule(inner, 0, PANEL_H - 4, lv_pct(100), false, PDA_RULE);

    /* control row: rounded dark plates with canvas-drawn icons, like Image 1 */
    int btn_y = PANEL_Y + PANEL_H + 16;
    int total_w = 9 * BTN_W + 6 * BTN_GAP + 2 * GROUP_GAP;
    int start_x = (800 - total_w) / 2;

    /* buttons: playlist, repeat, shuffle, vol-, vol+, prev, stop, play, next */
    static const icon_fn g_icons[9] = {
        draw_playlist_icon,
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
            x += (j == 2 || j == 4) ? GROUP_GAP : BTN_GAP;

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
        switch(i) {
        case 0:
            lv_obj_add_event_cb(b, on_folder_click, LV_EVENT_CLICKED, NULL);
            break;
        case 1:
            lv_obj_add_event_cb(b, on_loop_click, LV_EVENT_CLICKED, NULL);
            break;
        case 2:
            lv_obj_add_event_cb(b, on_shuffle_click, LV_EVENT_CLICKED, NULL);
            break;
        case 5:
            lv_obj_add_event_cb(b, on_prev_click, LV_EVENT_CLICKED, NULL);
            break;
        case 6:
            lv_obj_add_event_cb(b, on_stop_click, LV_EVENT_CLICKED, NULL);
            break;
        case 7:
            lv_obj_add_event_cb(b, on_play_click, LV_EVENT_CLICKED, NULL);
            break;
        case 8:
            lv_obj_add_event_cb(b, on_next_click, LV_EVENT_CLICKED, NULL);
            break;
        }

        /* the canvas: rounded plate + icon */
        uint32_t * buf = g_icon_bufs[i];
        memset(buf, 0, sizeof(uint32_t) * (size_t)BTN_W * (size_t)BTN_H);
         draw_plate_buf(buf, BTN_W, BTN_H, BTN_RADIUS,
                        RADIO_BTN_FILL, RADIO_BTN_EDGE);
         /* all icons light grey; alpha forced (the ARGB gotcha) */
         g_icons[i](buf, BTN_W, BTN_H, 0xFF000000u | RADIO_BTN_ICON);

        lv_obj_t * c = lv_canvas_create(b);
        lv_canvas_set_buffer(c, buf, BTN_W, BTN_H, LV_COLOR_FORMAT_ARGB8888);
        lv_obj_set_pos(c, 0, 0);
        lv_obj_set_size(c, BTN_W, BTN_H);
        lv_obj_set_style_bg_opa(c, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(c, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(c, 0, LV_PART_MAIN);
        lv_obj_set_clickable(c, false);
    }

    /* music manager: scrollable list of tracks in the selected folder */
    r->track_list = lv_obj_create(p);
    lv_obj_set_pos(r->track_list, 100, 204);
    lv_obj_set_size(r->track_list, 600, 260);
    lv_obj_set_style_bg_color(r->track_list, lv_color_hex(PDA_CHAT_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(r->track_list, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(r->track_list, lv_color_hex(PDA_CHAT_EDGE), LV_PART_MAIN);
    lv_obj_set_style_border_width(r->track_list, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(r->track_list, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_all(r->track_list, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_row(r->track_list, 2, LV_PART_MAIN);
    lv_obj_set_flex_flow(r->track_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollbar_mode(r->track_list, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scrollable(r->track_list, true);

    /* scan the directory */
    radio_scan_dir();
    radio_build_track_list();

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
    if(r->root) lv_kill_tree(r->root);
    r->root = NULL;
    r->panel = NULL;
    r->title_canvas = NULL;
    r->shuffle_canvas = NULL;
    r->time_canvas = NULL;
    r->loop_canvas = NULL;
    r->folder_canvas = NULL;
    r->track_list = NULL;
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
    radio_list_folders();
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

static void on_shuffle_click(lv_event_t * e)
{
    LV_UNUSED(e);
    g_radio.shuffle = !g_radio.shuffle;
    refresh();
}

static void on_loop_click(lv_event_t * e)
{
    LV_UNUSED(e);
    g_radio.loop = !g_radio.loop;
    refresh();
}

static void on_folder_click(lv_event_t * e)
{
    LV_UNUSED(e);
    radio_list_folders();
    if(g_radio.n_folders <= 0) return;
    g_radio.folder_idx++;
    if(g_radio.folder_idx >= g_radio.n_folders) g_radio.folder_idx = 0;
    if(g_radio.folder_idx < 0) g_radio.folder_idx = 0;
    const char * _mr3 = (const char *)(void *)g_radio.music_root;
    const char * _fn3 = (const char *)(void *)g_radio.folders[g_radio.folder_idx];
    snprintf(g_radio.music_dir, sizeof(g_radio.music_dir), "%s/%s", _mr3, _fn3);
    prefs_set_str(radio_dir_key(), g_radio.music_dir);
    radio_stop();
    g_radio.cur_track = -1;
    radio_scan_dir();
    radio_build_track_list();
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

    /* row 2: Folder: <name> */
    {
        char fbuf[256];
        snprintf(fbuf, sizeof(fbuf), "Folder: %s", folder_basename(g_radio.music_dir));
        text_canvas_set_text(g_radio.folder_canvas,
            fbuf, &pda_font_mono_10, PDA_MSG_TIME);
    }

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
