#include "ui/pda_shell.h"
#include "ui/palette.h"
#include "ui/chat_ui.h"
#include "ui/other_ui.h"
#include "ui/prefs.h"
#include "ui/i18n.h"
#include "ui/format.h"
#include "ui/grit.h"
#include "assets/fonts/pda_fonts.h"
#include "gps.h"
#include "chat/chat.h"
#include "poi.h"

#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define TAB_COUNT   7
#define HEADER_W    800
#define HEADER_H    58
/* the header is only as tall as it has to be: the map tab has the layer
   strip under the tabs, the other tabs have nothing there */
#define HEADER_H_TABS 34

#define MAP_VIEW_W  800
#define MAP_VIEW_H  411
#define MAP_VIEW_Y  69     /* the map's y offset inside the 800x480 screen */


#define SUB_COUNT   5
#define SUB_PITCH   148
#define SUB_X0      163
#define SUB_Y0      37
#define SUB_H       16
#define SUB_W       145
#define SUB_RISE    1
#define SUB_TAG_X   6
#define SUB_TAG_W   144
#define SUB_TAG_SLANT 11

#define TAB_PITCH   80
#define TAB_W       76
#define TAB_X0      120
#define TAB_Y0      13
#define TAB_H       20
#define TAB_SLANT   13


static const tr_id_t g_layer_ids[SUB_COUNT] = {
    TR_LAYER_TAG, TR_LAYER_AREAS, TR_LAYER_TRADERS, TR_LAYER_STASHES, TR_LAYER_WAYPOINTS
};

static const tr_id_t g_tab_ids[TAB_COUNT] = {
    TR_TAB_MAP, TR_TAB_MSG, TR_TAB_CONTACTS, TR_TAB_RADIO, TR_TAB_CAMERA,
    TR_TAB_WALLET, TR_TAB_OTHER
};

/* the tabs that are still stubs */
static const char * const g_panel_titles[TAB_COUNT - 1] = {
    "Messages", "Contacts", "Radio", "Camera", "Wallet", "Other"
};
static const char * const g_panel_bodies[TAB_COUNT - 1] = {
    "No new messages.",
    "No contacts yet.",
    "Signal: none",
    "No camera feed yet.",
    "Balance: 0 sats",
    "Nothing to tune yet."
};

typedef struct {
    uint16_t * buf;
    int w, h;
    int base;       /* zn at which 1 texel = 1 screen px (256 = master) */
} map_layer_t;

#define MAX_PACKS     8
#define MAX_PACK_LV   8
#define NUM_MIPS      3
#define REF_ZOOM      12
#define ZOOM_STEPS    16

typedef struct {
    double w_off_x, w_off_y;  /* world coord of source texel 0 */
    int w_exp;                /* W units per source texel = 2^w_exp */
    int w, h;
    uint16_t * map;
    size_t size;
    bool owned;               /* malloc'd (mip) vs mmap'ed file */
    bool real;                /* real level (not a synthesized mip) */
} map_level_t;

typedef struct {
    char name[24];
    double lon0, lon1, lat0, lat1;
    map_level_t lv[MAX_PACK_LV];
    int n_lv;
    bool mips_ready;
} map_pack_t;

typedef struct {
    lv_obj_t * root;
    lv_group_t * group;
    lv_obj_t * header;
    lv_obj_t * canvas;
    lv_obj_t * clock_lbl;
    lv_obj_t * title_lbl;
    lv_obj_t * tab_lbls[TAB_COUNT];
    lv_obj_t * tab_hit[TAB_COUNT];
    lv_obj_t * sub_lbls[SUB_COUNT];
    lv_obj_t * sub_hit[SUB_COUNT];
    lv_obj_t * view;
    lv_obj_t * map;
    lv_obj_t * map_canvas;
    lv_obj_t * overlay;
    lv_obj_t * zoom_btn[3];
    lv_obj_t * pack_lbl;
    lv_obj_t * poi_card;           /* the card of a tapped merchant */
    lv_obj_t * poi_card_name;
    lv_obj_t * poi_card_info;
    lv_timer_t * gps_timer;
    map_pack_t packs[MAX_PACKS];
    int n_packs;
    int active_pack;
    double lon_c, lat_c;
    uint16_t * view_buf;
    lv_obj_t * markers[24];
    double marker_lon[24];
    double marker_lat[24];
    int marker_size[24];
    int marker_layer[24];   /* the button of the second row it belongs to */
    int marker_count;
    int me_marker;          /* index of the player marker (follows the GPS) */
    gps_fix_t gps;
    int zoom_idx;
    lv_obj_t * panels[TAB_COUNT - 1];
    int active_tab;
    int focus_tab;
    int focus_sub;
    bool sub_on[SUB_COUNT];
    uint32_t canvas_buf[HEADER_W * HEADER_H];
    uint32_t icon_buf[28 * 28];    /* the recenter button's target */
} shell_t;

static shell_t g_sh;

/* ---------- canvas raster helpers (ARGB8888) ---------- */

static uint32_t argb_hex(uint32_t rgb)
{
    return 0xFF000000u | (rgb & 0xFFFFFFu);
}

static void px(int x, int y, uint32_t c)
{
    if(x < 0 || y < 0 || x >= HEADER_W || y >= HEADER_H) return;
    g_sh.canvas_buf[y * HEADER_W + x] = c;
}

static void fill_rect(int x, int y, int w, int h, uint32_t c)
{
    for(int j = y; j < y + h; j++)
        for(int i = x; i < x + w; i++)
            px(i, j, c);
}

static void stroke_rect(int x, int y, int w, int h, uint32_t c)
{
    for(int i = x; i < x + w; i++) { px(i, y, c); px(i, y + h - 1, c); }
    for(int j = y; j < y + h; j++) { px(x, j, c); px(x + w - 1, j, c); }
}

static void fill_circle(int cx, int cy, int r, uint32_t c)
{
    for(int j = cy - r; j <= cy + r; j++)
        for(int i = cx - r; i <= cx + r; i++) {
            int dx = i - cx, dy = j - cy;
            if(dx * dx + dy * dy <= r * r) px(i, j, c);
        }
}

static void stroke_circle(int cx, int cy, int r, uint32_t c)
{
    for(int j = cy - r - 1; j <= cy + r + 1; j++)
        for(int i = cx - r - 1; i <= cx + r + 1; i++) {
            int dx = i - cx, dy = j - cy;
            int d2 = dx * dx + dy * dy;
            int ro = (r + 1) * (r + 1), ri = (r - 1) * (r - 1);
            if(d2 <= ro && d2 >= ri) px(i, j, c);
        }
}

/* the little target on the recenter button: a ring with four ticks */
static void draw_recenter_icon(uint32_t * buf, int w, int h)
{
    memset(buf, 0, sizeof(uint32_t) * (size_t)w * (size_t)h);
    int cx = w / 2, cy = h / 2;
    uint32_t col = argb_hex(PDA_MAP_ME);
    for(int a = 0; a < 360; a += 2) {
        double r = a * M_PI / 180.0;
        int x = cx + (int)(7.0 * cos(r) + 0.5);
        int y = cy + (int)(7.0 * sin(r) + 0.5);
        if(x >= 0 && x < w && y >= 0 && y < h) buf[y * w + x] = col;
    }
    for(int d = -12; d <= 12; d++) {
        if(d > -5 && d < 5) continue;
        int x = cx + d, y = cy;
        if(x >= 0 && x < w) buf[y * w + x] = col;
        x = cx; y = cy + d;
        if(y >= 0 && y < h) buf[y * w + x] = col;
    }
}

static void draw_parallelogram(int ox, int oy, int w, int h, int slant,
                               uint32_t fill, uint32_t stroke)
{
    for(int y = 0; y < h; y++) {
        int shift = slant * (h - 1 - y) / (h - 1);
        int x1 = ox + shift;
        int x2 = ox + w - 1 + shift;
        if(y == 0 || y == h - 1) {
            for(int x = x1; x <= x2; x++) px(x, oy + y, stroke);
        }
        else {
            px(x1, oy + y, stroke);
            px(x2, oy + y, stroke);
            if(fill) {
                for(int x = x1 + 1; x < x2; x++) px(x, oy + y, fill);
            }
        }
    }
}

/* ---------- header painting ---------- */

static void apply_layers(shell_t * sh);   /* the markers of a layer */

static void redraw_header(void)
{
    shell_t * sh = &g_sh;

    memset(sh->canvas_buf, 0, sizeof(sh->canvas_buf));
    fill_rect(0, 0, HEADER_W, HEADER_H, argb_hex(PDA_BG));
    fill_rect(0, 6, HEADER_W, 1, argb_hex(PDA_TOP_LINE));
    fill_rect(0, TAB_Y0 + TAB_H - 1, HEADER_W, 1, argb_hex(PDA_BASELINE));

    for(int i = 0; i < TAB_COUNT; i++) {
        bool on = (i == sh->active_tab);
        draw_parallelogram(TAB_X0 + i * TAB_PITCH, TAB_Y0, TAB_W, TAB_H, TAB_SLANT,
                           argb_hex(PDA_TAB_FILL),
                           argb_hex(on ? PDA_TAB_ACTIVE_STROKE : PDA_TAB_STROKE));
    }

    if(sh->active_tab == 0) {
        draw_parallelogram(SUB_TAG_X, SUB_Y0, SUB_TAG_W, SUB_H, SUB_TAG_SLANT,
                           argb_hex(PDA_TAG_FILL), argb_hex(PDA_TAG_STROKE));
        for(int i = 1; i < SUB_COUNT; i++) {
            int x = SUB_X0 + (i - 1) * SUB_PITCH;
            int y = SUB_Y0 - (i - 1) * SUB_RISE;
            fill_rect(x, y, SUB_W, SUB_H, argb_hex(PDA_SUB_FILL));
            fill_rect(x, y, SUB_W, 1, argb_hex(PDA_SUB_EDGE));
            fill_rect(x, y + SUB_H - 1, SUB_W, 1, argb_hex(PDA_SUB_EDGE));
            int cx = x + 5, cy = y + SUB_H / 2;
            fill_circle(cx, cy, 5, argb_hex(PDA_SUB_DISC));
            fill_circle(cx, cy, sh->sub_on[i] ? 3 : 2,
                        argb_hex(sh->sub_on[i] ? PDA_SUB_DOT_ON : PDA_SUB_DOT_OFF));
        }
    }

    stroke_rect(696, 13, 26, 11, argb_hex(PDA_BATTERY));
    fill_rect(698, 15, 18, 7, argb_hex(PDA_BATTERY_FILL));

    if(sh->focus_tab >= 0) {
        int ox = TAB_X0 + sh->focus_tab * TAB_PITCH;
        draw_parallelogram(ox - 2, TAB_Y0 - 2, TAB_W + 4, TAB_H + 4, TAB_SLANT,
                           0, argb_hex(PDA_TAB_ACTIVE_STROKE));
        draw_parallelogram(ox - 1, TAB_Y0 - 1, TAB_W + 2, TAB_H + 2, TAB_SLANT,
                           0, argb_hex(PDA_TAB_ACTIVE_STROKE));
    }

    if(sh->focus_sub >= 1 && sh->active_tab == 0) {
        int x = SUB_X0 + (sh->focus_sub - 1) * SUB_PITCH;
        int y = SUB_Y0 - (sh->focus_sub - 1) * SUB_RISE;
        stroke_rect(x - 2, y - 2, SUB_W + 4, SUB_H + 4, argb_hex(PDA_TAB_ACTIVE_STROKE));
        stroke_rect(x - 1, y - 1, SUB_W + 2, SUB_H + 2, argb_hex(PDA_TAB_ACTIVE_STROKE));
    }

    lv_obj_invalidate(sh->canvas);
}

static void restyle_labels(void)
{
    shell_t * sh = &g_sh;
    int unread = chat_unread();

    for(int i = 0; i < TAB_COUNT; i++) {
        uint32_t col = (i == sh->active_tab) ? PDA_TAB_ACTIVE_TEXT : PDA_TAB_TEXT;
        /* the Messages tab carries the unread count */
        if(i == 1 && unread > 0) col = PDA_MSG_ACCENT;
        lv_obj_set_style_text_color(sh->tab_lbls[i], lv_color_hex(col), LV_PART_MAIN);
    }

    for(int i = 1; i < SUB_COUNT; i++)
        lv_obj_set_style_text_color(sh->sub_lbls[i],
                                    lv_color_hex(sh->sub_on[i] ? PDA_SUB_TEXT_ON
                                                               : PDA_TAB_TEXT),
                                    LV_PART_MAIN);

    lv_obj_set_style_text_color(sh->clock_lbl, lv_color_hex(PDA_CLOCK), LV_PART_MAIN);
    lv_obj_set_style_text_color(sh->title_lbl, lv_color_hex(PDA_TITLE), LV_PART_MAIN);
}

/* the Messages tab shows how many messages came in while it was not open */
static void update_unread(void)
{
    shell_t * sh = &g_sh;
    if(sh->active_tab == 1) chat_reset_unread();     /* it is open: read them */
    int unread = chat_unread();
    char buf[32];
    snprintf(buf, sizeof(buf), unread > 0 ? "Messages %d" : "Messages", unread);
    if(strcmp(lv_label_get_text(sh->tab_lbls[1]), buf) != 0)
        lv_label_set_text(sh->tab_lbls[1], buf);
    restyle_labels();
}

static void update_title(shell_t * sh)
{
    lv_label_set_text(sh->title_lbl, tr(g_tab_ids[sh->active_tab]));
}

/* the language changed: the words of the tab bar are put back */
static void reload_text(void)
{
    shell_t * sh = &g_sh;
    for(int i = 0; i < TAB_COUNT; i++)
        if(sh->tab_lbls[i]) lv_label_set_text(sh->tab_lbls[i], tr(g_tab_ids[i]));
    for(int i = 0; i < SUB_COUNT; i++)
        if(sh->sub_lbls[i]) lv_label_set_text(sh->sub_lbls[i], tr(g_layer_ids[i]));
    update_title(sh);
}

/* the theme changed: the overlay's buttons wear the new colours */
static void restyle_overlay(void)
{
    shell_t * sh = &g_sh;
    for(int i = 0; i < 3; i++) {
        lv_obj_t * b = sh->zoom_btn[i];
        if(!b) continue;
        lv_obj_set_style_bg_color(b, lv_color_hex(PDA_ZOOM_BG), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(b, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_color(b, lv_color_hex(PDA_ZOOM_BORDER), LV_PART_MAIN);
        lv_obj_set_style_text_color(b, lv_color_hex(PDA_TEXT), LV_PART_MAIN);
    }
    if(sh->pack_lbl)
        lv_obj_set_style_text_color(sh->pack_lbl, lv_color_hex(PDA_SUB_DOT_OFF),
                                    LV_PART_MAIN);
}

/* ---------- the clock ---------- */

static void update_clock(lv_timer_t * timer)
{
    LV_UNUSED(timer);
    char buf[16];
    fmt_clock(time(NULL), buf, sizeof(buf));
    if(getenv("PDA_DEBUG")) fprintf(stderr, "[clock] %s\n", buf);
    lv_label_set_text(g_sh.clock_lbl, buf);
}

/* ---------- GPS ---------- */

static void map_update(shell_t * sh);

static void update_gps(lv_timer_t * timer)
{
    LV_UNUSED(timer);
    shell_t * sh = &g_sh;
    gps_poll(&sh->gps);
    if(sh->me_marker >= 0 && sh->me_marker < sh->marker_count) {
        sh->marker_lon[sh->me_marker] = sh->gps.lon;
        sh->marker_lat[sh->me_marker] = sh->gps.lat;
    }
    if(getenv("PDA_DEBUG"))
        fprintf(stderr, "[gps] lat=%.6f lon=%.6f valid=%d sim=%d marker=%d\n",
                sh->gps.lat, sh->gps.lon, sh->gps.valid, sh->gps.simulated, sh->me_marker);
    if(sh->me_marker >= 0) map_update(sh);   /* move the player marker */
    other_ui_status(PDA_VERSION, gps_status(&sh->gps));
    update_unread();
}

/* ---------- view switching ---------- */

static void show_view(shell_t * sh)
{
    bool is_map = (sh->active_tab == 0);
    lv_obj_set_hidden(sh->map, !is_map);
    lv_obj_set_hidden(sh->overlay, !is_map);

    /* the header shrinks to the tab row when there is no layer strip under
       it, so the panel below starts right under the tabs */
    if(sh->header) {
        int hh = is_map ? HEADER_H : HEADER_H_TABS;
        if(lv_obj_get_height(sh->header) != hh) lv_obj_set_height(sh->header, hh);
    }

    for(int i = 0; i < TAB_COUNT - 1; i++) {
        if(i == sh->active_tab - 1) lv_obj_set_hidden(sh->panels[i], false);
        else lv_obj_set_hidden(sh->panels[i], true);
    }

    for(int i = 0; i < SUB_COUNT; i++) {
        lv_obj_set_hidden(sh->sub_lbls[i], !is_map);
        if(sh->sub_hit[i]) lv_obj_set_hidden(sh->sub_hit[i], !is_map);
    }
    if(!is_map) sh->focus_sub = -1;
}

static void set_tab(shell_t * sh, int idx)
{
    if(idx < 0 || idx >= TAB_COUNT) return;
    sh->active_tab = idx;
    if(idx == 1) chat_reset_unread();     /* the Messages tab is open now */
    redraw_header();
    restyle_labels();
    update_title(sh);
    show_view(sh);
}

static void on_sub_focus(lv_event_t * e)
{
    shell_t * sh = &g_sh;
    int i = (int)(lv_intptr_t)lv_event_get_user_data(e);
    if(lv_event_get_code(e) == LV_EVENT_FOCUSED) sh->focus_sub = i;
    else if(sh->focus_sub == i) sh->focus_sub = -1;
    redraw_header();
}

static void on_sub_click(lv_event_t * e)
{
    shell_t * sh = &g_sh;
    int i = (int)(lv_intptr_t)lv_event_get_user_data(e);
    sh->sub_on[i] = !sh->sub_on[i];   /* and the layer comes with it */
    apply_layers(sh);
    /* the card belongs to the traders: it goes when they do */
    if(i == 2 && sh->poi_card) lv_obj_set_hidden(sh->poi_card, true);
    redraw_header();
    restyle_labels();
}

static void on_tab_click(lv_event_t * e)
{
    set_tab(&g_sh, (int)(lv_intptr_t)lv_event_get_user_data(e));
}

static void on_tab_focus(lv_event_t * e)
{
    shell_t * sh = &g_sh;
    int i = (int)(lv_intptr_t)lv_event_get_user_data(e);
    if(lv_event_get_code(e) == LV_EVENT_FOCUSED) sh->focus_tab = i;
    else if(sh->focus_tab == i) sh->focus_tab = -1;
    redraw_header();
}

/* ---------- builders ---------- */

static lv_obj_t * make_hit(lv_obj_t * parent, lv_group_t * group,
                           int x, int y, int w, int h,
                           lv_event_cb_t cb, lv_event_cb_t focus_cb, void * user)
{
    lv_obj_t * hit = lv_obj_create(parent);
    lv_obj_set_pos(hit, x, y);
    lv_obj_set_size(hit, w, h);
    lv_obj_set_style_bg_opa(hit, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(hit, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(hit, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(hit, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(hit, LV_DIR_NONE);
    lv_obj_set_clickable(hit, true);
    lv_obj_add_event_cb(hit, cb, LV_EVENT_CLICKED, user);
    lv_obj_add_event_cb(hit, focus_cb, LV_EVENT_FOCUSED, user);
    lv_obj_add_event_cb(hit, focus_cb, LV_EVENT_DEFOCUSED, user);
    lv_group_add_obj(group, hit);
    return hit;
}

static void build_header(shell_t * sh, lv_obj_t * parent, lv_group_t * group)
{
    lv_obj_t * header = lv_obj_create(parent);
    lv_obj_set_size(header, HEADER_W, HEADER_H);
    lv_obj_set_style_bg_opa(header, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(header, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(header, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(header, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(header, LV_DIR_NONE);
    sh->header = header;

    sh->canvas = lv_canvas_create(header);
    lv_obj_set_pos(sh->canvas, 0, 0);
    lv_obj_set_size(sh->canvas, HEADER_W, HEADER_H);
    lv_canvas_set_buffer(sh->canvas, sh->canvas_buf, HEADER_W, HEADER_H, LV_COLOR_FORMAT_ARGB8888);

    for(int i = 0; i < TAB_COUNT; i++) {
        lv_obj_t * lbl = lv_label_create(header);
        lv_label_set_text(lbl, tr(g_tab_ids[i]));
        lv_obj_set_pos(lbl, TAB_X0 + i * TAB_PITCH, TAB_Y0 + 4);
        lv_obj_set_width(lbl, TAB_W);
        lv_label_set_long_mode(lbl, LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_set_style_text_font(lbl, &pda_font_mono_10, LV_PART_MAIN);
        sh->tab_lbls[i] = lbl;

        sh->tab_hit[i] = make_hit(header, group, TAB_X0 + i * TAB_PITCH, TAB_Y0,
                                  TAB_W, TAB_H, on_tab_click, on_tab_focus,
                                  (void *)(lv_intptr_t)i);
    }

    for(int i = 0; i < SUB_COUNT; i++) {
        lv_obj_t * lbl = lv_label_create(header);
        lv_label_set_text(lbl, tr(g_layer_ids[i]));
        if(i == 0) {
            lv_obj_set_pos(lbl, SUB_TAG_X, SUB_Y0 + 2);
            lv_obj_set_width(lbl, SUB_TAG_W);
            lv_obj_set_style_text_color(lbl, lv_color_hex(PDA_TAG_TEXT), LV_PART_MAIN);
        }
        else {
            int x = SUB_X0 + (i - 1) * SUB_PITCH;
            int y = SUB_Y0 - (i - 1) * SUB_RISE;
            lv_obj_set_pos(lbl, x, y + 3);
            lv_obj_set_width(lbl, SUB_W);
        }
        lv_label_set_long_mode(lbl, LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_set_style_text_font(lbl, &pda_font_mono_10, LV_PART_MAIN);
        sh->sub_lbls[i] = lbl;

        if(i > 0) {
            int x = SUB_X0 + (i - 1) * SUB_PITCH;
            int y = SUB_Y0 - (i - 1) * SUB_RISE;
            sh->sub_hit[i] = make_hit(header, group, x, y, SUB_W, SUB_H,
                                      on_sub_click, on_sub_focus,
                                      (void *)(lv_intptr_t)i);
        }
    }

    sh->title_lbl = lv_label_create(header);
    lv_label_set_text(sh->title_lbl, tr(g_tab_ids[0]));
    lv_obj_set_pos(sh->title_lbl, 16, 18);
    lv_obj_set_style_text_font(sh->title_lbl, &pda_font_mono_10, LV_PART_MAIN);

    sh->clock_lbl = lv_label_create(header);
    lv_label_set_text(sh->clock_lbl, "00:00");
    lv_obj_set_pos(sh->clock_lbl, 726, 14);
    lv_obj_set_width(sh->clock_lbl, 72);
    lv_obj_set_style_text_align(sh->clock_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(sh->clock_lbl, &pda_font_mono_14, LV_PART_MAIN);

    lv_timer_create(update_clock, 1000, NULL);
    int poll = atoi(prefs_str("gps_poll", "1000"));
    if(poll < 200) poll = 1000;
    sh->gps_timer = lv_timer_create(update_gps, poll, NULL);
}

/* ---------- map ---------- */

/* MAP PACKS
   ----------
   A pack is one region of the world stored as a folder:
       assets/maps/<name>/pack.cfg      bbox + one "level <z> <w> <h> <ox> <oy>" per level
       assets/maps/<name>/z<z>.rgb565   imagery, RGB565, row major
   The app auto-discovers every pack under the maps dir (override with the
   PDA_MAPS env var), so adding a region - or a deeper zoom level for an
   existing region - is just dropping files in a folder: no code change.

   All positions (markers, place names, view centre) are geographic, so they
   stay correct across packs and levels. The world coordinate "W" is the global
   pixel at REF_ZOOM; one W unit = zn/256 screen px, so the zoom ladder means
   the same thing for every pack regardless of its levels. */

static const uint16_t g_zoom_levels[ZOOM_STEPS] = {
    64, 96, 128, 170, 256, 341, 512, 683, 1024, 1365, 2048, 2730, 4096, 5461, 8192,
    10923
};

static double geo_w_x(double lon)
{
    return (lon + 180.0) / 360.0 * (double)(1ull << REF_ZOOM) * 256.0;
}

static double geo_w_y(double lat)
{
    double r = lat * M_PI / 180.0;
    return (1.0 - log(tan(r) + 1.0 / cos(r)) / M_PI) / 2.0 * (double)(1ull << REF_ZOOM) * 256.0;
}

static double geo_lat_from_w(double w)
{
    double n = M_PI * (1.0 - 2.0 * w / ((double)(1ull << REF_ZOOM) * 256.0));
    return atan(sinh(n)) * 180.0 / M_PI;
}

static double geo_lon_from_w(double w)
{
    return w / ((double)(1ull << REF_ZOOM) * 256.0) * 360.0 - 180.0;
}

static double geo_w_per_deg_lon(void)
{
    return (double)(1ull << REF_ZOOM) * 256.0 / 360.0;
}

/* place / area names - geographic, so they show up in every pack that
   covers them. tier: 0 always, 1 zoom >= 128, 2 zoom >= 256
   style: 0 town, 1 region/geo area, 2 major/island */
typedef struct {
    const char * name;
    double lon, lat;
    uint8_t tier, style;
    int8_t dx, dy;
} map_label_t;

static const map_label_t g_map_labels[] = {
    /* tier 0 - always visible */
    {"FUNCHAL",               -16.9193, 32.6478, 0, 2, 46,  0},
    {"PAUL DA SERRA",         -17.1400, 32.7520, 0, 1, 0,   0},
    {"PICO RUIVO",            -16.9422, 32.7569, 0, 1, -24, -14},
    {"PONTA DE SAO LOURENCO", -16.6900, 32.7260, 0, 1, 0,  -22},
    {"DESERTAS",              -16.5400, 32.5600, 0, 2, 0,   0},
    {"PORTO SANTO",           -16.3100, 33.0450, 0, 2, 0,   0},
    /* tier 1 - zoom >= 128 */
    {"CAMARA DE LOBOS",       -17.0128, 32.6497, 1, 0, 0, -10},
    {"RIBEIRA BRAVA",         -17.0528, 32.6719, 1, 0, 0, -10},
    {"PONTA DO SOL",          -17.0999, 32.6800, 1, 0, 0, -10},
    {"CALHETA",               -17.1700, 32.7167, 1, 0, 0,  20},
    {"PONTA DO PARGO",        -17.2514, 32.7858, 1, 0, 0, -10},
    {"PORTO MONIZ",           -17.0669, 32.8697, 1, 0, 0, -10},
    {"SAO VICENTE",           -17.0497, 32.7883, 1, 0, 0, -10},
    {"SANTANA",               -16.8864, 32.8183, 1, 0, 0, -10},
    {"MACHICO",               -16.7667, 32.7314, 1, 0, 0, -10},
    /* tier 2 - zoom >= 256 */
    {"VILA BALEIRA",          -16.3378, 33.0642, 2, 0, 0,  14},
    {"SANTA CRUZ",            -16.7836, 32.6925, 2, 0, 0,  14},
    {"CANICO",                -16.8439, 32.6611, 2, 0, 0, -10},
    {"CAMACHA",               -16.7706, 32.6906, 2, 0, 0, -12},
    {"CURRAL DAS FREIRAS",    -16.9722, 32.7114, 2, 0, 0, -24},
    {"MONTE",                 -16.9036, 32.6786, 2, 0, 0, -10},
    {"RIBEIRO FRIO",          -16.9042, 32.7406, 2, 0, 0, -10},
    {"PICO DO ARIEIRO",       -16.9267, 32.7161, 2, 0, 0, -10},
    {"FAIAL",                 -16.8606, 32.8017, 2, 0, 0, -10},
    {"CANICAL",               -16.6956, 32.7394, 2, 0, 0,  14},
    {"AIRPORT",               -16.7745, 32.6979, 2, 0, 0, -18},
};

#define LABEL_COUNT (sizeof(g_map_labels) / sizeof(g_map_labels[0]))
static const uint16_t g_label_min_zn[3] = { 0, 128, 256 };

/* ---------- pack loading ---------- */

static uint16_t * map_file_map(const char * path, size_t * size_out)
{
    int fd = open(path, O_RDONLY);
    if(fd < 0) return NULL;
    struct stat st;
    if(fstat(fd, &st) != 0 || st.st_size <= 0) { close(fd); return NULL; }
    void * p = mmap(NULL, (size_t)st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    if(p == MAP_FAILED) return NULL;
    *size_out = (size_t)st.st_size;
    return (uint16_t *)p;
}

static void cfg_trim(char * s)
{
    size_t n = strlen(s);
    while(n && (s[n - 1] == '\n' || s[n - 1] == '\r' || s[n - 1] == ' ' || s[n - 1] == '\t'))
        s[--n] = 0;
    char * p = s;
    while(*p == ' ' || *p == '\t') p++;
    if(p != s) memmove(s, p, strlen(p) + 1);
}

static bool cfg_get(const char * line, const char * key, char * out, size_t outn)
{
    const char * p = strstr(line, "=");
    if(!p) return false;
    size_t klen = (size_t)(p - line);
    while(klen && (line[klen - 1] == ' ' || line[klen - 1] == '\t')) klen--;
    if(klen != strlen(key) || strncmp(line, key, klen) != 0) return false;
    p++;
    while(*p == ' ' || *p == '\t') p++;
    snprintf(out, outn, "%s", p);
    cfg_trim(out);
    return true;
}

static void load_packs(shell_t * sh)
{
    const char * root = getenv("PDA_MAPS");
    if(!root) root = "assets/maps";
    DIR * d = opendir(root);
    if(!d) return;

    struct dirent * e;
    while((e = readdir(d)) && sh->n_packs < MAX_PACKS) {
        if(e->d_name[0] == '.') continue;
        char dir_path[512], cfg_path[544], line[512];
        snprintf(dir_path, sizeof(dir_path), "%s/%s", root, e->d_name);
        snprintf(cfg_path, sizeof(cfg_path), "%s/pack.cfg", dir_path);
        FILE * f = fopen(cfg_path, "r");
        if(!f) continue;

        map_pack_t pk;
        memset(&pk, 0, sizeof(pk));
        snprintf(pk.name, sizeof(pk.name), "%.*s", (int)sizeof(pk.name) - 1, e->d_name);
        int lz[8], lw[8], lh[8], lox[8], loy[8], n = 0;
        while(fgets(line, sizeof(line), f)) {
            int z, W, H, ox, oy;
            if(sscanf(line, "level %d %d %d %d %d", &z, &W, &H, &ox, &oy) == 5 && n < 8) {
                lz[n] = z; lw[n] = W; lh[n] = H; lox[n] = ox; loy[n] = oy; n++;
                continue;
            }
            char val[256];
            if(cfg_get(line, "name", val, sizeof(val)))
                snprintf(pk.name, sizeof(pk.name), "%.*s", (int)sizeof(pk.name) - 1, val);
            else if(cfg_get(line, "lon0", val, sizeof(val))) pk.lon0 = atof(val);
            else if(cfg_get(line, "lon1", val, sizeof(val))) pk.lon1 = atof(val);
            else if(cfg_get(line, "lat0", val, sizeof(val))) pk.lat0 = atof(val);
            else if(cfg_get(line, "lat1", val, sizeof(val))) pk.lat1 = atof(val);
        }
        fclose(f);
        if(n == 0) continue;

        for(int i = 0; i < n; i++) {
            char path[640];
            snprintf(path, sizeof(path), "%s/z%d.rgb565", dir_path, lz[i]);
            size_t size = 0;
            uint16_t * buf = map_file_map(path, &size);
            if(!buf) continue;
            if(size != (size_t)lw[i] * lh[i] * 2) { munmap(buf, size); continue; }
            map_level_t * lv = &pk.lv[pk.n_lv++];
            lv->w = lw[i];
            lv->h = lh[i];
            lv->w_exp = REF_ZOOM - lz[i];
            lv->w_off_x = (double)lox[i] * ldexp(1.0, lv->w_exp);
            lv->w_off_y = (double)loy[i] * ldexp(1.0, lv->w_exp);
            lv->map = buf;
            lv->size = size;
            lv->owned = false;
            lv->real = true;
        }
        if(pk.n_lv == 0) continue;
        sh->packs[sh->n_packs++] = pk;
    }
    closedir(d);
}

/* box-downsample src by 2x2 (RGB565 component average) into dst */
static void build_mip(const uint16_t * src, int sw, int sh,
                      uint16_t * dst, int dw, int dh)
{
    for(int y = 0; y < dh; y++) {
        const uint16_t * r0 = src + (long)(2 * y) * sw;
        const uint16_t * r1 = (2 * y + 1 < sh) ? r0 + sw : r0;
        for(int x = 0; x < dw; x++) {
            int x0 = 2 * x, x1 = (2 * x + 1 < sw) ? x0 + 1 : x0;
            uint16_t a = r0[x0], b = r0[x1], c = r1[x0], d = r1[x1];
            uint32_t rr = ((a >> 11) + (b >> 11) + (c >> 11) + (d >> 11) + 2) / 4;
            uint32_t gg = (((a >> 5) & 63) + ((b >> 5) & 63) +
                           ((c >> 5) & 63) + ((d >> 5) & 63) + 2) / 4;
            uint32_t bb = ((a & 31) + (b & 31) + (c & 31) + (d & 31) + 2) / 4;
            dst[y * dw + x] = (uint16_t)((rr << 11) | (gg << 5) | bb);
        }
    }
}

/* synthesize downsample levels from the finest real level of a pack */
static void build_pack_mips(map_pack_t * pk)
{
    if(pk->mips_ready) return;
    pk->mips_ready = true;

    int fine = 0;
    for(int i = 0; i < pk->n_lv; i++)
        if(pk->lv[i].real && pk->lv[i].w_exp < pk->lv[fine].w_exp) fine = i;

    int w = pk->lv[fine].w, h = pk->lv[fine].h, exp = pk->lv[fine].w_exp;
    uint16_t * cur = pk->lv[fine].map;
    for(int m = 0; m < NUM_MIPS && pk->n_lv < MAX_PACK_LV; m++) {
        int nw = w / 2, nh = h / 2;
        if(nw < 2 || nh < 2) break;
        uint16_t * dst = malloc((size_t)nw * nh * 2);
        if(!dst) break;
        build_mip(cur, w, h, dst, nw, nh);
        map_level_t * lv = &pk->lv[pk->n_lv++];
        lv->w = nw;
        lv->h = nh;
        lv->w_exp = exp + 1;
        lv->w_off_x = pk->lv[fine].w_off_x;
        lv->w_off_y = pk->lv[fine].w_off_y;
        lv->map = dst;
        lv->size = (size_t)nw * nh * 2;
        lv->owned = true;
        lv->real = false;
        cur = dst;
        w = nw;
        h = nh;
        exp++;
    }
}

/* finest level of the pack that is not downsampled at this zoom */
static const map_level_t * pick_pack_level(const map_pack_t * pk, int zn)
{
    const map_level_t * best = NULL;
    double zscale = zn / 256.0;
    for(int i = 0; i < pk->n_lv; i++) {
        const map_level_t * lv = &pk->lv[i];
        if(!lv->map) continue;
        double step = ldexp(1.0, lv->w_exp);
        if(step * zscale < 1.0) continue;              /* would downsample */
        if(!best) { best = lv; continue; }
        double bs = ldexp(1.0, best->w_exp);
        if(step < bs || (step == bs && lv->real && !best->real)) best = lv;
    }
    if(!best) {                                       /* zoom too low: use finest */
        for(int i = 0; i < pk->n_lv; i++) {
            const map_level_t * lv = &pk->lv[i];
            if(!lv->map) continue;
            if(!best || ldexp(1.0, lv->w_exp) < ldexp(1.0, best->w_exp)) best = lv;
        }
    }
    return best;
}

/* the step (W units per texel) a pack would resolve to at this zoom, counting
   the mip levels it would synthesize. Used to compare packs fairly, since a
   pack whose mips are not built yet must not look coarser than one that has. */
static double pack_best_step(const map_pack_t * pk, int zn, bool * from_mip)
{
    double zscale = zn / 256.0;
    double finest = 0;
    bool have = false;
    if(from_mip) *from_mip = false;
    for(int i = 0; i < pk->n_lv; i++) {
        if(!pk->lv[i].map) continue;
        double step = ldexp(1.0, pk->lv[i].w_exp);
        if(!have || step < finest) { finest = step; have = true; }
    }
    if(!have) return 0;

    double best = 0;
    bool found = false;
    for(int i = 0; i < pk->n_lv; i++) {
        if(!pk->lv[i].map) continue;
        double step = ldexp(1.0, pk->lv[i].w_exp);
        if(step * zscale >= 1.0 && (!found || step < best)) { best = step; found = true; }
    }
    if(!found) best = finest;
    return best;
}

/* ---------- markers ---------- */

/* the shape of a marker */
enum { MARK_RING, MARK_CIRCLE, MARK_SQUARE };

static lv_obj_t * make_marker(shell_t * sh, lv_obj_t * map, double lon, double lat,
                              int size, uint32_t color, int shape, int layer)
{
    lv_obj_t * o = lv_obj_create(map);
    lv_obj_set_pos(o, 0, 0);
    lv_obj_set_size(o, size, size);
    if(shape == MARK_RING) {
        lv_obj_set_style_radius(o, 50, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(o, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(o, 2, LV_PART_MAIN);
        lv_obj_set_style_border_color(o, lv_color_hex(color), LV_PART_MAIN);
    }
    else {
        lv_obj_set_style_bg_color(o, lv_color_hex(color), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(o, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(o, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(o, shape == MARK_CIRCLE ? 50 : 0, LV_PART_MAIN);
    }
    lv_obj_set_style_pad_all(o, 0, LV_PART_MAIN);
    lv_obj_set_scrollable(o, false);
    lv_obj_set_clickable(o, false);
    if(sh->marker_count < 24) {
        int i = sh->marker_count++;
        sh->markers[i] = o;
        sh->marker_lon[i] = lon;
        sh->marker_lat[i] = lat;
        sh->marker_size[i] = size;
        sh->marker_layer[i] = layer;
    }
    return o;
}

/* what the buttons of the second row show on the map.
   The layer numbers are the numbers of the buttons (1..4). */
typedef struct {
    int layer;
    const char * name;
    double lon, lat;
    int size;
    pda_col_id_t color;      /* the palette is a runtime thing: an id, not a colour */
    int shape;
} map_point_t;

static const map_point_t g_map_points[] = {
    /* Areas - the two big ones */
    { 1, "Curral das Freiras",    -16.9722, 32.7114, 26, PDA_C_MAP_AREA,      MARK_RING },
    { 1, "Ponta de Sao Lourenco", -16.6900, 32.7260, 26, PDA_C_MAP_AREA,      MARK_RING },
    /* Stashes */
    { 3, "Calheta",               -17.1700, 32.7167, 13, PDA_C_MAP_STASH,     MARK_SQUARE },
    { 3, "Pico Ruivo",            -16.9422, 32.7569, 13, PDA_C_MAP_STASH,     MARK_SQUARE },
    /* Waypoints */
    { 4, "Airport",               -16.7745, 32.6979, 16, PDA_C_MAP_IMPORTANT, MARK_RING },
    { 4, "Porto Santo",           -16.3100, 33.0450, 16, PDA_C_MAP_IMPORTANT, MARK_RING },
};

#define MAP_POINT_COUNT (sizeof(g_map_points) / sizeof(g_map_points[0]))

/* the markers of a layer are shown when its button is on */
static void apply_layers(shell_t * sh)
{
    for(int i = 0; i < sh->marker_count; i++) {
        int layer = sh->marker_layer[i];
        if(layer < 0) continue;                 /* the player is always there */
        lv_obj_set_hidden(sh->markers[i], !sh->sub_on[layer]);
    }
}

/* ---------- the traders (BTC Map) ---------- */

/* The merchants are drawn into the map canvas: there are hundreds of them
 * and an LVGL object each would be too much. The icon is a circle around a
 * bag of coins, in a desaturated blue. */

#define POI_ICON      18      /* the icon is 18x18 */
#define POI_CLUSTER   16      /* merchants this close are one icon */
#define POI_VISIBLE   1024    /* the most that are drawn at once */

typedef struct {
    int x, y, idx;
} poi_screen_t;

static poi_screen_t s_poi_screen[POI_VISIBLE];

/* the bag of coins in the middle of the circle */
static const char * const g_bag_bitmap[9] = {
    "..###..",
    "..###..",
    ".#####.",
    "#######",
    "#######",
    "#######",
    ".#####.",
    "..###..",
    "..###..",
};

static uint16_t rgb565_of(uint32_t rgb)
{
    return (uint16_t)(((rgb >> 19 & 31) << 11) | ((rgb >> 10 & 63) << 5) |
                      (rgb >> 3 & 31));
}

static void view_px(shell_t * sh, int x, int y, uint16_t c)
{
    if(x < 0 || y < 0 || x >= MAP_VIEW_W || y >= MAP_VIEW_H) return;
    sh->view_buf[(long)y * MAP_VIEW_W + x] = c;
}

/* the circle around the bag of coins */
static void draw_trader_icon(shell_t * sh, int cx, int cy, uint16_t col)
{
    for(int j = 0; j < POI_ICON; j++) {
        for(int i = 0; i < POI_ICON; i++) {
            double dx = i + 0.5 - POI_ICON / 2.0;
            double dy = j + 0.5 - POI_ICON / 2.0;
            double d = sqrt(dx * dx + dy * dy);
            if(d < 6.8 || d > 8.6) continue;
            view_px(sh, cx + i - POI_ICON / 2, cy + j - POI_ICON / 2, col);
        }
    }
    for(int j = 0; j < 9; j++) {
        const char * row = g_bag_bitmap[j];
        for(int i = 0; i < 9; i++) {
            if(row[i] != '#') continue;
            view_px(sh, cx + i - 4, cy + j - 4, col);
        }
    }
}

/* where a place is on the map, in the map's own coordinates */
static void poi_screen_pos(shell_t * sh, int zn, const poi_place_t * p, int * x, int * y)
{
    double zscale = zn / 256.0;
    *x = (int)((geo_w_x(p->lon) - geo_w_x(sh->lon_c)) * zscale + 400.0);
    *y = (int)((geo_w_y(p->lat) - geo_w_y(sh->lat_c)) * zscale + 205.0);
}

/* the merchants of the Traders layer. The ones that stand close together
 * are one icon, or the city would be a blue blob. */
static void draw_poi_layer(shell_t * sh, int zn)
{
    int n = poi_count();
    if(n <= 0) return;

    int m = 0;
    for(int i = 0; i < n && m < POI_VISIBLE; i++) {
        const poi_place_t * p = poi_get(i);
        int x, y;
        poi_screen_pos(sh, zn, p, &x, &y);
        if(x < -POI_ICON || x > MAP_VIEW_W + POI_ICON) continue;
        if(y < -POI_ICON || y > MAP_VIEW_H + POI_ICON) continue;
        s_poi_screen[m].x = x;
        s_poi_screen[m].y = y;
        s_poi_screen[m].idx = i;
        m++;
    }
    if(m <= 0) return;

    uint16_t col = rgb565_of(PDA_MAP_TRADER);
    static char taken[POI_VISIBLE];
    memset(taken, 0, (size_t)m);

    for(int i = 0; i < m; i++) {
        if(taken[i]) continue;
        double sx = 0, sy = 0;
        int c = 0;
        for(int j = 0; j < m; j++) {
            if(taken[j]) continue;
            double dx = s_poi_screen[j].x - s_poi_screen[i].x;
            double dy = s_poi_screen[j].y - s_poi_screen[i].y;
            if(dx * dx + dy * dy > (double)POI_CLUSTER * POI_CLUSTER) continue;
            taken[j] = 1;
            sx += s_poi_screen[j].x;
            sy += s_poi_screen[j].y;
            c++;
        }
        draw_trader_icon(sh, (int)(sx / c), (int)(sy / c), col);
    }
}

/* ---------- rendering ---------- */

static void blit_level(shell_t * sh, const map_level_t * lv, int x0, int y0, int x1, int y1)
{
    if(!lv->map) return;
    if(x0 < 0) x0 = 0;
    if(y0 < 0) y0 = 0;
    if(x1 > MAP_VIEW_W - 1) x1 = MAP_VIEW_W - 1;
    if(y1 > MAP_VIEW_H - 1) y1 = MAP_VIEW_H - 1;
    if(x0 > x1 || y0 > y1) return;

    double zscale = g_zoom_levels[sh->zoom_idx] / 256.0;   /* screen px per W unit */
    double inv = ldexp(1.0, -lv->w_exp);                     /* source texels per W unit */
    double wc = geo_w_x(sh->lon_c);
    double wct = geo_w_y(sh->lat_c);
    /* the viewport spans (x1-x0)/zscale W units -> (x1-x0)/zscale * inv source texels,
       i.e. the source advances inv/zscale texels per screen pixel */
    int64_t sstep = (int64_t)llround(inv / zscale * 4294967296.0);
    int64_t sx = (int64_t)llround((wc + (x0 - 400) / zscale - lv->w_off_x) * inv * 4294967296.0);
    int64_t sy = (int64_t)llround((wct + (y0 - 205) / zscale - lv->w_off_y) * inv * 4294967296.0);

    if(getenv("PDA_DEBUG")) {
        int64_t ex = sx + (int64_t)(x1 - x0) * sstep;
        int64_t ey = sy + (int64_t)(y1 - y0) * sstep;
        fprintf(stderr, "[blit] zn=%d exp=%d inv=%g zscale=%g img=%dx%d  srcx=[%ld..%ld] srcy=[%ld..%ld]\n",
                g_zoom_levels[sh->zoom_idx], lv->w_exp, inv, zscale, lv->w, lv->h,
                (long)(sx >> 32), (long)(ex >> 32), (long)(sy >> 32), (long)(ey >> 32));
    }
    for(int j = y0; j <= y1; j++) {
        int ty = (int)(sy >> 32);
        if(ty < 0) ty = 0; else if(ty >= lv->h) ty = lv->h - 1;
        const uint16_t * srow = lv->map + (long)ty * lv->w;
        int64_t sxr = sx;
        uint16_t * d = sh->view_buf + (long)j * MAP_VIEW_W;
        for(int i = x0; i <= x1; i++) {
            int tx = (int)(sxr >> 32);
            if(tx < 0) tx = 0; else if(tx >= lv->w) tx = lv->w - 1;
            d[i] = srow[tx];
            sxr += sstep;
        }
        sy += sstep;
    }
}

static void draw_map_labels(shell_t * sh, int zn)
{
    if(!sh->map_canvas) return;
    double zscale = zn / 256.0;
    double wc = geo_w_x(sh->lon_c);
    double wct = geo_w_y(sh->lat_c);

    lv_layer_t layer;
    lv_canvas_init_layer(sh->map_canvas, &layer);
    for(uint32_t i = 0; i < LABEL_COUNT; i++) {
        const map_label_t * lb = &g_map_labels[i];
        if(zn < g_label_min_zn[lb->tier]) continue;
        int sx = (int)((geo_w_x(lb->lon) - wc) * zscale + 400.0) + lb->dx;
        int sy = (int)((geo_w_y(lb->lat) - wct) * zscale + 205.0) + lb->dy;
        if(sx < -160 || sx > MAP_VIEW_W + 160 || sy < -40 || sy > MAP_VIEW_H + 40) continue;
        const lv_font_t * font = (lb->style == 2) ? &pda_font_mono_14 : &pda_font_mono_10;
        int ls = (lb->style == 0) ? 0 : 2;
        uint32_t col = (lb->style == 0) ? PDA_MAP_LABEL_TOWN :
                       (lb->style == 1 ? PDA_MAP_LABEL_REG : PDA_MAP_LABEL_MAJOR);
        lv_point_t tsz;
        lv_text_get_size(&tsz, lb->name, font, ls, 0, LV_COORD_MAX, LV_TEXT_ALIGN_LEFT);
        int tx = sx - tsz.x / 2, ty = sy - tsz.y / 2;
        lv_draw_label_dsc_t dsc;
        lv_draw_label_dsc_init(&dsc);
        dsc.text = lb->name;
        dsc.font = font;
        dsc.letter_space = ls;
        dsc.opa = LV_OPA_COVER;
        lv_area_t a;
        dsc.color = lv_color_hex(PDA_MAP_LABEL_SHADOW);
        a.x1 = (int32_t)tx + 1; a.y1 = (int32_t)ty + 1;
        a.x2 = (int32_t)(tx + tsz.x); a.y2 = (int32_t)(ty + tsz.y);
        lv_draw_label(&layer, &dsc, &a);
        dsc.color = lv_color_hex(col);
        a.x1 = (int32_t)tx; a.y1 = (int32_t)ty;
        lv_draw_label(&layer, &dsc, &a);
    }
    lv_canvas_finish_layer(sh->map_canvas, &layer);
}

static void map_update(shell_t * sh)
{
    if(!sh->map_canvas) return;

    int zn = g_zoom_levels[sh->zoom_idx];
    double zscale = zn / 256.0;

    /* background pack = the one covering the most area */
    int bg = -1;
    double bg_area = -1.0;
    for(int i = 0; i < sh->n_packs; i++) {
        map_pack_t * pk = &sh->packs[i];
        double area = fabs((pk->lon1 - pk->lon0) * (pk->lat1 - pk->lat0));
        if(area > bg_area) { bg_area = area; bg = i; }
    }

    if(bg >= 0) build_pack_mips(&sh->packs[bg]);

    /* best pack = finest level available for the current view centre */
    int best = -1;
    double best_step = 0, best_area = 0;
    bool best_mip = false;
    for(int i = 0; i < sh->n_packs; i++) {
        map_pack_t * pk = &sh->packs[i];
        if(sh->lon_c < pk->lon0 || sh->lon_c > pk->lon1) continue;
        if(sh->lat_c < pk->lat1 || sh->lat_c > pk->lat0) continue;
        const map_level_t * lv = pick_pack_level(pk, zn);
        if(!lv) continue;
        bool mip = false;
        double step = ldexp(1.0, lv->w_exp);
        /* a pack that would have to be minified is not worth showing */
        if(step * zscale < 1.0) continue;
        mip = !lv->real;
        double area = fabs((pk->lon1 - pk->lon0) * (pk->lat1 - pk->lat0));
        bool better = best < 0 || step < best_step;
        if(!better && step == best_step) {
            /* same resolution: prefer native imagery over a downsampled mip,
               then the more specific (smaller) region */
            if(mip != best_mip) better = mip < best_mip;
            else if(area < best_area) better = true;
        }
        if(better) {
            best = i;
            best_step = step;
            best_area = area;
            best_mip = mip;
        }
    }
    sh->active_pack = best >= 0 ? best : bg;

    if(bg >= 0) {
        build_pack_mips(&sh->packs[bg]);   /* smooth low zoom, built once */
        const map_level_t * lv = pick_pack_level(&sh->packs[bg], zn);
        if(lv) blit_level(sh, lv, 0, 0, MAP_VIEW_W - 1, MAP_VIEW_H - 1);
    }
    if(best >= 0 && best != bg) {
        map_pack_t * pk = &sh->packs[best];
        const map_level_t * lv = pick_pack_level(pk, zn);
        if(lv) {
            double wc = geo_w_x(sh->lon_c);
            double wct = geo_w_y(sh->lat_c);
            int x0 = (int)((geo_w_x(pk->lon0) - wc) * zscale + 400.0);
            int x1 = (int)((geo_w_x(pk->lon1) - wc) * zscale + 400.0);
            int y0 = (int)((geo_w_y(pk->lat0) - wct) * zscale + 205.0);
            int y1 = (int)((geo_w_y(pk->lat1) - wct) * zscale + 205.0);
            blit_level(sh, lv, x0, y0, x1, y1);
        }
    }

    /* the traders of the BTC Map pack, under the names of the places */
    if(sh->sub_on[2]) draw_poi_layer(sh, zn);

    if(sh->n_packs) draw_map_labels(sh, zn);

    double wc = geo_w_x(sh->lon_c);
    double wct = geo_w_y(sh->lat_c);
    for(int i = 0; i < sh->marker_count; i++) {
        int mx = (int)((geo_w_x(sh->marker_lon[i]) - wc) * zscale + 400.0) - sh->marker_size[i] / 2;
        int my = (int)((geo_w_y(sh->marker_lat[i]) - wct) * zscale + 205.0) - sh->marker_size[i] / 2;
        lv_obj_set_pos(sh->markers[i], mx, my);
    }

    if(getenv("PDA_DEBUG")) {
        for(int i = 0; i < sh->n_packs; i++) {
            const map_level_t * lv = pick_pack_level(&sh->packs[i], zn);
            fprintf(stderr, "[pack] %-14.14s levels=%d best=%s step=%g contains=%d area=%g\n",
                    sh->packs[i].name, sh->packs[i].n_lv,
                    lv ? "yes" : "no", lv ? ldexp(1.0, lv->w_exp) : 0.0,
                    (sh->lon_c >= sh->packs[i].lon0 && sh->lon_c <= sh->packs[i].lon1 &&
                     sh->lat_c <= sh->packs[i].lat0 && sh->lat_c >= sh->packs[i].lat1),
                    (sh->packs[i].lon1 - sh->packs[i].lon0) * (sh->packs[i].lat1 - sh->packs[i].lat0));
        }
        fprintf(stderr, "[pack] active=%-.14s zn=%d\n",
                sh->active_pack >= 0 ? sh->packs[sh->active_pack].name : "none", zn);
    }
    if(sh->pack_lbl && sh->active_pack >= 0) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%s  z%d", sh->packs[sh->active_pack].name,
                 REF_ZOOM - pick_pack_level(&sh->packs[sh->active_pack], zn)->w_exp);
        lv_label_set_text(sh->pack_lbl, buf);
    }

    if(getenv("PDA_DEBUG")) {
        uint32_t sum = 0;
        for(int i = 0; i < MAP_VIEW_W * MAP_VIEW_H; i += 997) sum += sh->view_buf[i];
        fprintf(stderr, "[map_update] done zn=%d active=%s bufsum=%u\n", zn,
                sh->active_pack >= 0 ? sh->packs[sh->active_pack].name : "none", sum);
    }
    lv_obj_invalidate(sh->map_canvas);
}

/* Zoom by one step. (ax, ay) is the anchor in map coordinates: the world
   point under it stays under it. Pass the screen centre (400, 205) to zoom
   around the middle of the view (what the +/- buttons do). */
static void map_zoom(shell_t * sh, int dir, int ax, int ay)
{
    int ni = sh->zoom_idx + dir;
    if(ni < 0 || ni >= ZOOM_STEPS) return;
    double old_scale = g_zoom_levels[sh->zoom_idx] / 256.0;
    double new_scale = g_zoom_levels[ni] / 256.0;
    double wx = geo_w_x(sh->lon_c) + (ax - 400) / old_scale;
    double wy = geo_w_y(sh->lat_c) + (ay - 205) / old_scale;
    sh->zoom_idx = ni;
    sh->lon_c = geo_lon_from_w(wx - (ax - 400) / new_scale);
    sh->lat_c = geo_lat_from_w(wy - (ay - 205) / new_scale);
    map_update(sh);
}

static void on_zin(lv_event_t * e)
{
    LV_UNUSED(e);
    map_zoom(&g_sh, +1, 400, 205);
}

static void on_zout(lv_event_t * e)
{
    LV_UNUSED(e);
    map_zoom(&g_sh, -1, 400, 205);
}

/* put the view back on the blue dot */
static void on_recenter(lv_event_t * e)
{
    LV_UNUSED(e);
    shell_t * sh = &g_sh;
    sh->lon_c = sh->gps.lon;
    sh->lat_c = sh->gps.lat;
    map_update(sh);
}

static lv_point_t g_last_pt;

/* the mouse wheel is handled in main.c (SDL level) because LVGL 9 routes the
   wheel to the focused object; we want it to zoom towards the cursor. */
void pda_shell_wheel(int diff, int x, int y)
{
    shell_t * sh = &g_sh;
    if(diff == 0) return;
    if(sh->active_tab != 0) return;               /* the map is on screen */
    int ax = x;
    int ay = y - MAP_VIEW_Y;
    if(ax < 0 || ax >= MAP_VIEW_W || ay < 0 || ay >= MAP_VIEW_H) return;
    map_zoom(sh, diff > 0 ? +1 : -1, ax, ay);
}

static void on_map_press(lv_event_t * e)
{
    LV_UNUSED(e);
    lv_indev_t * indev = lv_indev_active();
    if(indev) lv_indev_get_point(indev, &g_last_pt);
}

/* what a merchant takes, in words */
static void poi_flags_text(const poi_place_t * p, char * buf, size_t n)
{
    char tmp[112];
    size_t used = 0;
    tmp[0] = 0;

    /* append a word to the list */
    for(int i = 0; i < 6; i++) {
        const char * w = NULL;
        char year[16];
        switch(i) {
            case 0: if(p->flags & POI_F_LIGHTNING) w = "lightning"; break;
            case 1: if(p->flags & POI_F_CONTACTLESS) w = "lightning contactless"; break;
            case 2: if(p->flags & POI_F_ONCHAIN) w = "on-chain"; break;
            case 3: if(p->flags & POI_F_ATM) w = "bitcoin ATM"; break;
            case 4: if(p->flags & POI_F_COMPANION) w = "needs an app"; break;
            case 5:
                if(p->flags & POI_F_VERIFIED) {
                    snprintf(year, sizeof(year), "checked %u", (unsigned)p->year);
                    w = year;
                }
                break;
        }
        if(!w) continue;
        int len = snprintf(tmp + used, sizeof(tmp) - used, "%s%s", used ? ", " : "", w);
        if(len < 0 || (size_t)len >= sizeof(tmp) - used) break;
        used += len;
    }
    snprintf(buf, n, "%s", tmp);
}

/* the merchant under the finger, if there is one */
static int poi_under(int mx, int my)
{
    shell_t * sh = &g_sh;
    if(!sh->sub_on[2]) return -1;
    int n = poi_count();
    int best = -1;
    double best_d = 14.0 * 14.0;
    for(int i = 0; i < n; i++) {
        const poi_place_t * p = poi_get(i);
        int x, y;
        poi_screen_pos(sh, g_zoom_levels[sh->zoom_idx], p, &x, &y);
        double dx = x - mx, dy = y - my;
        double d = dx * dx + dy * dy;
        if(d < best_d) {
            best_d = d;
            best = i;
        }
    }
    return best;
}

/* a tap on the map opens the card of the merchant that is there */
static void on_map_click(lv_event_t * e)
{
    LV_UNUSED(e);
    shell_t * sh = &g_sh;
    if(!sh->poi_card) return;

    lv_indev_t * indev = lv_indev_active();
    if(!indev) return;
    lv_point_t p;
    lv_indev_get_point(indev, &p);
    int mx = p.x;
    int my = p.y - MAP_VIEW_Y;

    int i = poi_under(mx, my);
    if(i < 0) {
        lv_obj_set_hidden(sh->poi_card, true);
        return;
    }

    const poi_place_t * place = poi_get(i);
    char flags[96];
    poi_flags_text(place, flags, sizeof(flags));

    char line[160];
    snprintf(line, sizeof(line), "%s%s%s", place->name[0] ? place->name : "(no name)",
             place->address[0] ? " - " : "", place->address);
    lv_label_set_text(sh->poi_card_name, line);
    snprintf(line, sizeof(line), "%s%s%s", flags,
             place->category[0] ? " - " : "", place->category);
    lv_label_set_text(sh->poi_card_info, line);
    lv_obj_set_hidden(sh->poi_card, false);
    lv_obj_move_foreground(sh->poi_card);
}

static void on_map_pressing(lv_event_t * e)
{
    shell_t * sh = &g_sh;
    if(!sh->n_packs) return;
    lv_indev_t * indev = lv_indev_active();
    if(!indev) return;
    lv_point_t p;
    lv_indev_get_point(indev, &p);
    double zscale = g_zoom_levels[sh->zoom_idx] / 256.0;
    /* zscale = screen px per W unit, so one screen px = 1/zscale W units.
       The map must follow the finger 1:1 at every zoom level. */
    if(sh->poi_card && !lv_obj_is_hidden(sh->poi_card)) {
        /* the map is moving: the card would be about a place that is gone */
        lv_obj_set_hidden(sh->poi_card, true);
    }
    double dw = -(p.x - g_last_pt.x) / zscale;
    double dh = -(p.y - g_last_pt.y) / zscale;
    sh->lon_c += dw / geo_w_per_deg_lon();
    sh->lat_c = geo_lat_from_w(geo_w_y(sh->lat_c) + dh);
    g_last_pt = p;
    map_update(sh);
}

static void build_map(shell_t * sh, lv_obj_t * parent)
{
    lv_obj_t * map = lv_obj_create(parent);
    lv_obj_set_pos(map, 0, 0);
    lv_obj_set_size(map, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(map, lv_color_hex(PDA_MAP_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(map, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(map, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(map, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(map, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(map, LV_DIR_NONE);
    lv_obj_set_scrollable(map, false);
    lv_obj_set_clickable(map, true);
    sh->map = map;

    load_packs(sh);
    /* the merchants of BTC Map (the Traders layer) */
    poi_load(NULL);

    sh->view_buf = malloc(MAP_VIEW_W * MAP_VIEW_H * 2);
    sh->map_canvas = lv_canvas_create(map);
    lv_canvas_set_buffer(sh->map_canvas, sh->view_buf, MAP_VIEW_W, MAP_VIEW_H,
                         LV_COLOR_FORMAT_RGB565);
    lv_obj_set_pos(sh->map_canvas, 0, 0);
    if(!sh->n_packs) {
        uint16_t bg = (uint16_t)(((PDA_MAP_BG >> 19 & 31) << 11) |
                                 ((PDA_MAP_BG >> 10 & 63) << 5) | (PDA_MAP_BG >> 3 & 31));
        for(int i = 0; i < MAP_VIEW_W * MAP_VIEW_H; i++) sh->view_buf[i] = bg;
        lv_obj_t * lbl = lv_label_create(map);
        lv_label_set_text(lbl, "map data not found");
        lv_obj_center(lbl);
        lv_obj_set_style_text_color(lbl, lv_color_hex(PDA_TAB_TEXT), LV_PART_MAIN);
    }
    map_update(sh);

    /* markers (geographic) - the player and the layers of the second row */
    /* the player marker sits on the GPS position (see update_gps) */
    sh->me_marker = sh->marker_count;
    make_marker(sh, map, sh->gps.lon, sh->gps.lat, 28, PDA_MAP_ME, MARK_RING, -1);
    make_marker(sh, map, sh->gps.lon, sh->gps.lat, 16, PDA_MAP_ME, MARK_CIRCLE, -1);

    /* and the layers: areas, traders, stashes, waypoints */
    for(size_t i = 0; i < MAP_POINT_COUNT; i++) {
        const map_point_t * p = &g_map_points[i];
        make_marker(sh, map, p->lon, p->lat, p->size, pda_col(p->color), p->shape, p->layer);
    }
    apply_layers(sh);

    /* overlay: zoom buttons + attribution, fixed over the map */
    lv_obj_t * overlay = lv_obj_create(parent);
    lv_obj_set_pos(overlay, 0, 0);
    lv_obj_set_size(overlay, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(overlay, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(overlay, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(overlay, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(overlay, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scrollable(overlay, false);
    lv_obj_set_clickable(overlay, false);
    sh->overlay = overlay;

    lv_obj_t * zin = lv_button_create(overlay);
    lv_obj_set_pos(zin, 756, 334);
    lv_obj_set_size(zin, 28, 28);
    lv_obj_t * zin_lbl = lv_label_create(zin);
    lv_label_set_text(zin_lbl, "+");
    lv_obj_center(zin_lbl);
    lv_obj_add_event_cb(zin, on_zin, LV_EVENT_CLICKED, NULL);

    lv_obj_t * zout = lv_button_create(overlay);
    lv_obj_set_pos(zout, 756, 370);
    lv_obj_set_size(zout, 28, 28);
    lv_obj_t * zout_lbl = lv_label_create(zout);
    lv_label_set_text(zout_lbl, "-");
    lv_obj_center(zout_lbl);
    lv_obj_add_event_cb(zout, on_zout, LV_EVENT_CLICKED, NULL);

    lv_obj_t * zbtns[3] = { zin, zout, NULL };
    sh->zoom_btn[0] = zin;
    sh->zoom_btn[1] = zout;

    /* the recenter button: put the view back on the blue dot */
    draw_recenter_icon(sh->icon_buf, 28, 28);
    lv_obj_t * rc = lv_button_create(overlay);
    lv_obj_set_pos(rc, 756, 298);
    lv_obj_set_size(rc, 28, 28);
    lv_obj_set_style_pad_all(rc, 0, LV_PART_MAIN);   /* the icon fills it */
    lv_obj_t * ico = lv_canvas_create(rc);
    lv_canvas_set_buffer(ico, sh->icon_buf, 28, 28, LV_COLOR_FORMAT_ARGB8888);
    lv_obj_set_pos(ico, 0, 0);
    lv_obj_set_size(ico, 28, 28);
    lv_obj_add_event_cb(rc, on_recenter, LV_EVENT_CLICKED, NULL);
    zbtns[2] = rc;
    sh->zoom_btn[2] = rc;
    for(int i = 0; i < 3; i++) {
        lv_obj_set_style_bg_color(zbtns[i], lv_color_hex(PDA_ZOOM_BG), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(zbtns[i], LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(zbtns[i], 1, LV_PART_MAIN);
        lv_obj_set_style_border_color(zbtns[i], lv_color_hex(PDA_ZOOM_BORDER), LV_PART_MAIN);
        lv_obj_set_style_text_color(zbtns[i], lv_color_hex(PDA_TEXT), LV_PART_MAIN);
        lv_obj_set_style_radius(zbtns[i], 2, LV_PART_MAIN);
    }

    lv_obj_t * att = lv_label_create(overlay);
    lv_label_set_text(att, "(c) OSM  Imagery (c) Esri  BTC Map");
    lv_obj_align(att, LV_ALIGN_BOTTOM_LEFT, 8, -4);
    lv_obj_set_style_text_font(att, &pda_font_mono_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(att, lv_color_hex(PDA_SUB_DOT_OFF), LV_PART_MAIN);

    sh->pack_lbl = lv_label_create(overlay);
    lv_label_set_text(sh->pack_lbl, "");
    lv_obj_set_pos(sh->pack_lbl, 8, 4);
    lv_obj_set_style_text_font(sh->pack_lbl, &pda_font_mono_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(sh->pack_lbl, lv_color_hex(PDA_SUB_DOT_OFF), LV_PART_MAIN);

    /* the card of a merchant: it opens when one is tapped */
    lv_obj_t * card = lv_obj_create(overlay);
    lv_obj_set_pos(card, 8, MAP_VIEW_H - 62);
    lv_obj_set_size(card, 420, 46);
    lv_obj_set_style_bg_color(card, lv_color_hex(PDA_CHAT_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(card, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(card, lv_color_hex(PDA_CHAT_EDGE), LV_PART_MAIN);
    lv_obj_set_style_radius(card, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, 4, LV_PART_MAIN);
    lv_obj_set_clickable(card, false);
    lv_obj_set_hidden(card, true);
    sh->poi_card = card;

    sh->poi_card_name = lv_label_create(card);
    lv_label_set_text(sh->poi_card_name, "");
    lv_obj_set_pos(sh->poi_card_name, 2, 2);
    lv_obj_set_width(sh->poi_card_name, 404);
    lv_label_set_long_mode(sh->poi_card_name, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(sh->poi_card_name, &pda_font_mono_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(sh->poi_card_name, lv_color_hex(PDA_MSG_ACCENT), LV_PART_MAIN);

    sh->poi_card_info = lv_label_create(card);
    lv_label_set_text(sh->poi_card_info, "");
    lv_obj_set_pos(sh->poi_card_info, 2, 15);
    lv_obj_set_width(sh->poi_card_info, 404);
    lv_label_set_long_mode(sh->poi_card_info, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(sh->poi_card_info, &pda_font_mono_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(sh->poi_card_info, lv_color_hex(PDA_MSG_SYS), LV_PART_MAIN);

    lv_obj_add_event_cb(map, on_map_press, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(map, on_map_pressing, LV_EVENT_PRESSING, NULL);
    lv_obj_add_event_cb(map, on_map_click, LV_EVENT_CLICKED, NULL);

    map_update(sh);   /* position markers into viewport coords */
}
static lv_obj_t * build_panel(lv_obj_t * parent, const char * title, const char * body)
{
    lv_obj_t * p = lv_obj_create(parent);
    lv_obj_set_pos(p, 0, 0);
    lv_obj_set_size(p, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(p, lv_color_hex(PDA_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(p, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(p, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(p, 16, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(p, 8, LV_PART_MAIN);
    lv_obj_set_flex_flow(p, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollbar_mode(p, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scrollable(p, false);

    lv_obj_t * t = lv_label_create(p);
    lv_label_set_text(t, title);
    lv_obj_set_style_text_font(t, &pda_font_mono_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(t, lv_color_hex(PDA_TEXT), LV_PART_MAIN);

    lv_obj_t * b = lv_label_create(p);
    lv_label_set_text(b, body);
    lv_obj_set_style_text_font(b, &pda_font_mono_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(b, lv_color_hex(PDA_TEXT), LV_PART_MAIN);

    return p;
}

lv_obj_t * pda_shell_create(lv_obj_t * parent, lv_group_t * group)
{
    shell_t * sh = &g_sh;
    memset(sh, 0, sizeof(*sh));
    sh->root = parent;
    sh->active_tab = 0;
    sh->focus_tab = -1;
    sh->focus_sub = -1;
    sh->sub_on[1] = true;          /* the areas are on at boot */
    sh->me_marker = -1;
    gps_init(&sh->gps);
    sh->zoom_idx = 4;            /* 256 = 1 world texel per screen px */
    /* the view starts on the GPS position (gps_init falls back to Funchal
       when there is no fix yet) */
    sh->lon_c = sh->gps.lon;
    sh->lat_c = sh->gps.lat;
    sh->active_pack = -1;

    prefs_init();      /* the saved settings, before anything reads them */
    chat_init();

    /* the settings that change how everything looks and reads */
    pda_theme_load();
    tr_load();
    fmt_apply();
    grit_set_grain(!strcmp(prefs_str("grain", "on"), "on"));

    sh->group = group;

    lv_obj_set_style_bg_color(parent, lv_color_hex(PDA_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_text_font(parent, &pda_font_mono_14, LV_PART_MAIN);
    lv_obj_set_style_pad_all(parent, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollbar_mode(parent, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(parent, LV_DIR_NONE);

    build_header(sh, parent, group);

    sh->view = lv_obj_create(parent);
    lv_obj_set_width(sh->view, lv_pct(100));
    lv_obj_set_flex_grow(sh->view, 1);
    lv_obj_set_style_bg_opa(sh->view, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(sh->view, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(sh->view, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(sh->view, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scrollable(sh->view, false);

    build_map(sh, sh->view);

    for(int i = 0; i < TAB_COUNT - 1; i++) {
        if(i == 0) sh->panels[i] = chat_ui_create(sh->view, group);   /* the real one */
        else if(i == TAB_COUNT - 2) sh->panels[i] = other_ui_create(sh->view, group);
        else sh->panels[i] = build_panel(sh->view, g_panel_titles[i], g_panel_bodies[i]);
    }

    redraw_header();
    restyle_labels();
    show_view(sh);

    /* the tab the device opens on: the test hook wins over the setting */
    const char * start_tab = getenv("PDA_START_TAB");
    if(start_tab && *start_tab) set_tab(sh, atoi(start_tab));
    else {
        int tab = atoi(prefs_str("start_tab", "0"));
        if(tab < 0 || tab >= TAB_COUNT) tab = 0;
        set_tab(sh, tab);
    }

    return parent;
}

/* ---------- the theme and the language changed ---------- */

/* the panels are built with the colours and the words, so they are built
   again; the map and the header are painted again instead */
void pda_shell_retheme(void)
{
    shell_t * sh = &g_sh;

    redraw_header();
    restyle_labels();
    reload_text();
    restyle_overlay();
    map_update(sh);

    int page = 0, sub = 0;
    other_ui_get_state(&page, &sub);

    for(int i = 0; i < TAB_COUNT - 1; i++) {
        if(!sh->panels[i]) continue;
        if(i == 0) chat_ui_destroy();
        if(i == TAB_COUNT - 2) other_ui_destroy();
        lv_obj_delete(sh->panels[i]);
        sh->panels[i] = NULL;
    }
    for(int i = 0; i < TAB_COUNT - 1; i++) {
        if(i == 0) sh->panels[i] = chat_ui_create(sh->view, sh->group);
        else if(i == TAB_COUNT - 2) sh->panels[i] = other_ui_create(sh->view, sh->group);
        else sh->panels[i] = build_panel(sh->view, g_panel_titles[i], g_panel_bodies[i]);
    }

    other_ui_set_state(page, sub);
    show_view(sh);
}

/* how often the GPS is looked at */
void pda_shell_set_gps_poll(int ms)
{
    if(ms < 200) ms = 200;
    if(g_sh.gps_timer) lv_timer_set_period(g_sh.gps_timer, ms);
}
