#include "ui/other_ui.h"
#include "ui/palette.h"
#include "ui/prefs.h"
#include "ui/lvutil.h"
#include "ui/grit.h"
#include "ui/i18n.h"
#include "ui/format.h"
#include "ui/pda_shell.h"
#include "ui/chat_ui.h"
#include "assets/fonts/pda_fonts.h"
#include "chat/chat.h"
#include "identity/identity.h"
#include "gps.h"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netpacket/packet.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* The Other tab.
 *
 * The STALKER "guide" look: a gritty surface, a framed panel, two columns
 * of entries on the left and the page on the right.
 *
 *   +--------------------------------------------------+
 *   |                  Connections                     |  <- the strip
 *   +--------------------------------------------------+
 *   | Settings    |  Info      |  NETWORKING            |
 *   | Connections |  Bitchat   |  BIT ......... ONLINE   |
 *   | Extras      |  F.I.P.S   |  Nostr relay ... OFFLINE|
 *   |             |  WiFi      |                        |
 *   +--------------------------------------------------+
 *
 * Settings, Connections and Extras are the sections; the User page lives
 * under Settings now. Every one of them works, the Extras page is still
 * empty.
 */

#define PAGE_COUNT   3
#define NAV_W        160          /* the sections column */
#define NAV_H        22
#define NAV_PITCH    26
#define SUB_W        100          /* the sub tabs column, vertical */
#define SUB_H        20
#define SUB_PITCH    24
#define STRIP_H      26           /* the title strip under the tab bar */
#define FRAME_X      6
#define FRAME_Y      32
#define FRAME_W      788
#define FRAME_H      407
#define COL_X        14           /* the first column, inside the frame */
#define RULE1_X      182
#define SUB_X        190
#define RULE2_X      298
#define PAGE_X       306
#define PAGE_Y       38
#define PAGE_W       480
#define PAGE_H       395
#define ROW_H        18
#define FIELD_W      220
#define FIELD_H      17
#define VAL_X        150
#define VAL_PAD      5      /* where a value's text starts: the same place the
                               text of a field starts inside its box (the box's
                               border is 1px and its padding 4px) */
#define VAL_Y        3      /* the value sits on the row's baseline, with the
                               name on the left */
#define BACK_W       800
#define BACK_H       452
/* the mesh map on the Bitchat page */
#define MESH_W       440
#define MESH_H       250
#define MESH_NODES   8

enum { PAGE_SETTINGS = 0, PAGE_CONNECTIONS = 1, PAGE_EXTRAS = 2 };

/* the sections and their sub tabs */
static const tr_id_t g_page_ids[PAGE_COUNT] = {
    TR_SEC_SETTINGS, TR_SEC_CONNECTIONS, TR_SEC_EXTRAS
};

static const tr_id_t g_sub_ids[PAGE_COUNT][8] = {
    { TR_SUB_GENERAL, TR_SUB_ADVANCED, TR_SUB_LANGUAGE, TR_SUB_THEMES, TR_SUB_SYSTEM,
      TR_SUB_USER },
    { TR_SUB_INFO, TR_SUB_BITCHAT, TR_SUB_FIPS, TR_SUB_WIFI, TR_SUB_GPS },
    { 0 },
};

static void other_ui_refresh(void);
static void page_user(void);
static void no_theme_transition(lv_obj_t * o);
static void other_ui_on_name(lv_event_t * e);
static void other_ui_on_room(lv_event_t * e);
static void on_save_name(lv_event_t * e);
static void on_save_room(lv_event_t * e);
static void on_save_channel(lv_event_t * e);
static void other_ui_on_channel(lv_event_t * e);
static void save_relay(lv_obj_t * ta);
static void other_ui_on_relay(lv_event_t * e);
static void on_save_relay(lv_event_t * e);
static void build_content(void);
static void on_key_toggle(lv_event_t * e);
static void on_new_click(lv_event_t * e);
static void other_ui_on_paste(lv_event_t * e);
static void on_paste_click(lv_event_t * e);
static lv_obj_t * save_button(lv_obj_t * row_obj, lv_event_cb_t cb);
static void on_toggle_click(lv_event_t * e);
static void on_reset_log(lv_event_t * e);

typedef struct {
    lv_obj_t * root;
    lv_group_t * group;
    lv_obj_t * nav_box[PAGE_COUNT];
    lv_obj_t * nav_lbl[PAGE_COUNT];
    lv_obj_t * sub_btn[8];
    lv_obj_t * sub_lbl[8];
    int n_subs;
    lv_obj_t * content;
    lv_obj_t * strip_title;
    lv_timer_t * timer;
    int page;
    int sub;
    int sec_no;            /* the headings alternate their colour */

    /* the live labels of the page that is on screen */
    lv_obj_t * live[16];
    int n_live;
    char live_key[16][24];   /* what each live label shows */

    /* the toggles on the User page */
    lv_obj_t * tog[4];
    char tog_key[4][24];
    int n_tog;

    /* the mesh map on the Bitchat page */
    lv_obj_t * mesh_canvas;
    uint32_t mesh_buf[MESH_W * MESH_H];
    lv_obj_t * bit_hint;      /* the firewall hint, shown when nobody answers */

    lv_obj_t * name_ta;
    lv_obj_t * room_ta;
    lv_obj_t * chan_ta;
    lv_obj_t * chan_btn;
    lv_obj_t * chan_btn_lbl;
    lv_obj_t * name_btn;
    lv_obj_t * room_btn;
    lv_obj_t * name_btn_lbl;
    lv_obj_t * room_btn_lbl;

    /* the relay URL row on the Info page (the nstr carrier's one setting) */
    lv_obj_t * relay_ta;
    lv_obj_t * relay_btn;
    lv_obj_t * relay_btn_lbl;

    /* the identity rows under IDENTITY: the nstr key, the reveal block,
       [KEY] / [NEW] / [PASTE] and the import field */
    lv_obj_t * key_btn_lbl;
    lv_obj_t * new_btn_lbl;
    lv_obj_t * paste_btn_lbl;
    lv_obj_t * paste_ta;
} other_ui_t;

static other_ui_t g_ui;

/* the nstr reveal on the User page: up only until anything else happens
   (navigation, retheme, leaving the Other tab - other_ui_hide_secrets is
   the shell's hook for that last one), and never at boot */
static bool s_show_key = false;
/* the second press of [NEW] confirms an irreversible regeneration */
static bool s_new_armed = false;

static char s_version[32] = "";
static char s_gps[64] = "";

void other_ui_status(const char * version, const char * gps)
{
    snprintf(s_version, sizeof(s_version), "%s", version ? version : "");
    snprintf(s_gps, sizeof(s_gps), "%s", gps ? gps : "");
}

/* ---------- small helpers ---------- */

static lv_obj_t * plain(lv_obj_t * parent, int w, int h)
{
    lv_obj_t * o = lv_obj_create(parent);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_bg_opa(o, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(o, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(o, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(o, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(o, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(o, LV_DIR_NONE);
    lv_obj_set_scrollable(o, false);
    return o;
}

static lv_obj_t * label(lv_obj_t * parent, int x, int y, int w, const char * text,
                        const lv_font_t * font, uint32_t color)
{
    lv_obj_t * l = lv_label_create(parent);
    lv_label_set_text(l, text);
    lv_obj_set_pos(l, x, y);
    if(w > 0) lv_obj_set_width(l, w);
    lv_label_set_long_mode(l, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(l, font, LV_PART_MAIN);
    lv_obj_set_style_text_color(l, lv_color_hex(color), LV_PART_MAIN);
    lv_obj_set_style_pad_all(l, 0, LV_PART_MAIN);
    return l;
}

/* a section heading: a small square, the title and a hairline under it */
static void section(lv_obj_t * parent, const char * title)
{
    uint32_t col = (g_ui.sec_no++ % 2) ? PDA_HEAD_B : PDA_HEAD_A;

    lv_obj_t * s = plain(parent, lv_pct(100), 16);
    lv_obj_set_flex_grow(s, 0);

    lv_obj_t * sq = plain(s, 6, 6);
    lv_obj_set_pos(sq, 0, 5);
    lv_obj_set_style_bg_color(sq, lv_color_hex(col), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(sq, LV_OPA_COVER, LV_PART_MAIN);

    lv_obj_t * l = label(s, 12, 0, 0, title, &pda_font_mono_10, col);
    lv_obj_set_height(l, 12);

    /* the hairline under the heading */
    grit_rule(s, 0, 15, lv_pct(100), false, PDA_RULE);
}

/* one "setting" row: a name on the left, the value on the right */
static lv_obj_t * row(lv_obj_t * parent, const char * name)
{
    lv_obj_t * r = plain(parent, lv_pct(100), ROW_H);
    lv_obj_set_flex_grow(r, 0);
    label(r, 0, VAL_Y, 150, name, &pda_font_mono_10, PDA_MSG_SYS);
    /* the hairline between the rows */
    grit_rule(r, 0, ROW_H - 1, lv_pct(100), false, PDA_RULE);
    return r;
}

static lv_obj_t * value(lv_obj_t * r, const char * text, uint32_t color)
{
    /* the text starts inside the value column, on the row's baseline - the
       same place the text of a field starts inside its box */
    lv_obj_t * v = label(r, VAL_X + VAL_PAD, VAL_Y, 280, text, &pda_font_mono_10, color);
    return v;
}

/* a row whose value changes on its own */
static lv_obj_t * live_row(lv_obj_t * parent, const char * name, const char * key,
                           const char * text, uint32_t color)
{
    lv_obj_t * r = row(parent, name);
    lv_obj_t * v = value(r, text, color);
    if(g_ui.n_live < 16) {
        g_ui.live[g_ui.n_live] = v;
        snprintf(g_ui.live_key[g_ui.n_live], sizeof(g_ui.live_key[0]), "%s", key);
        g_ui.n_live++;
    }
    return r;
}

/* a text field in a row: the cursor only shows while the field has the
   focus, so an empty field shows its placeholder whole */
static lv_obj_t * text_field(lv_obj_t * row_obj, const char * placeholder)
{
    lv_obj_t * ta = lv_textarea_create(row_obj);
    lv_textarea_set_one_line(ta, true);
    /* one_line makes the label as wide as the text and gives it a minimum of
       100% of the field - a minimum that is bigger than the field, so the
       field ends up the size of its text and the first letter is cut off */
    lv_obj_t * tl = lv_obj_get_child(ta, 0);
    if(tl) {
        lv_obj_set_width(tl, lv_pct(100));
        lv_obj_set_style_min_width(tl, 0, 0);
        /* the text sits at the left of the field: the theme would centre it */
        lv_obj_set_style_text_align(tl, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    }
    lv_obj_set_style_text_align(ta, LV_TEXT_ALIGN_LEFT, LV_PART_TEXTAREA_PLACEHOLDER);
    lv_textarea_set_placeholder_text(ta, placeholder);
    lv_obj_set_pos(ta, VAL_X, 0);
    lv_obj_set_size(ta, FIELD_W, FIELD_H);
    /* and it must not shrink either */
    lv_obj_set_style_min_width(ta, FIELD_W, LV_PART_MAIN);
    lv_obj_set_style_bg_color(ta, lv_color_hex(PDA_CHAT_INPUT), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ta, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(ta, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(ta, lv_color_hex(PDA_CHAT_EDGE), LV_PART_MAIN);
    lv_obj_set_style_text_font(ta, &pda_font_mono_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(ta, lv_color_hex(PDA_MSG_TEXT), LV_PART_MAIN);
    lv_obj_set_style_text_color(ta, lv_color_hex(PDA_MSG_SYS), LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_pad_all(ta, 2, LV_PART_MAIN);
    /* a little more room on the left: the letters have a bearing of their
       own and the first one was cut in half by the edge of the field.
       The box's border is 1px, so this puts the text of the field exactly
       where a plain value starts - one VAL_PAD inside the column. */
    lv_obj_set_style_pad_left(ta, VAL_PAD - 1, LV_PART_MAIN);
    lv_caret_init(ta);
    return ta;
}

/* a small square toggle */
static lv_obj_t * toggle(lv_obj_t * parent, bool on)
{
    lv_obj_t * b = lv_button_create(parent);
    lv_obj_set_size(b, 14, 14);
    lv_obj_set_flex_grow(b, 0);
    lv_obj_set_style_bg_color(b, lv_color_hex(on ? PDA_MSG_ACCENT : PDA_CHAT_EDGE),
                              LV_PART_MAIN);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(b, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(b, lv_color_hex(PDA_CHAT_EDGE), LV_PART_MAIN);
    lv_obj_set_style_radius(b, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(b, 0, LV_PART_MAIN);
    lv_no_transition(b);
    return b;
}

/* ---------- the network state, for the Info page ---------- */

typedef struct {
    const char * name;
    bool up;
    const char * detail;
} net_state_t;

static int collect_states(net_state_t * out, int max)
{
    int n = 0;
    for(int i = 0; i < chat_transport_count() && n < max; i++) {
        const chat_transport_t * tr = chat_transport_get(i);
        if(!tr) continue;
        out[n].name = tr->label;
        out[n].up = (tr->state == CHAT_CARRIER_ONLINE);
        out[n].detail = tr->status;
        n++;
    }
    return n;
}

/* the wifi interface: the first one that is up and has an address */
static bool wifi_info(char * name, size_t namen, char * ip, size_t ipn, char * mac,
                      size_t macn)
{
    struct ifaddrs * ifa = NULL;
    bool found = false;
    if(getifaddrs(&ifa) != 0) return false;

    for(struct ifaddrs * p = ifa; p; p = p->ifa_next) {
        if(!p->ifa_addr || p->ifa_addr->sa_family != AF_INET) continue;
        if(p->ifa_flags & IFF_LOOPBACK) continue;
        struct sockaddr_in * sa = (struct sockaddr_in *)p->ifa_addr;
        if(!sa->sin_addr.s_addr) continue;
        snprintf(name, namen, "%s", p->ifa_name);
        snprintf(ip, ipn, "%s", inet_ntoa(sa->sin_addr));
        found = true;
        break;
    }

    if(found && mac && macn) {
        for(struct ifaddrs * p = ifa; p; p = p->ifa_next) {
            if(!p->ifa_addr || p->ifa_addr->sa_family != AF_PACKET) continue;
            if(strcmp(p->ifa_name, name) != 0) continue;
            struct sockaddr_ll * ll = (struct sockaddr_ll *)p->ifa_addr;
            snprintf(mac, macn, "%02x:%02x:%02x:%02x:%02x:%02x",
                     ll->sll_addr[0], ll->sll_addr[1], ll->sll_addr[2],
                     ll->sll_addr[3], ll->sll_addr[4], ll->sll_addr[5]);
            break;
        }
    }

    freeifaddrs(ifa);
    return found;
}

static bool wifi_up(void)
{
    struct ifaddrs * ifa = NULL;
    bool up = false;
    if(getifaddrs(&ifa) != 0) return false;
    for(struct ifaddrs * p = ifa; p; p = p->ifa_next) {
        if(!p->ifa_addr || p->ifa_addr->sa_family != AF_INET) continue;
        if(p->ifa_flags & IFF_LOOPBACK) continue;
        if((p->ifa_flags & IFF_UP) && (p->ifa_flags & IFF_RUNNING)) up = true;
    }
    freeifaddrs(ifa);
    return up;
}

/* ---------- the pages ---------- */

static void page_info(void)
{
    net_state_t st[8];
    int n = collect_states(st, 8);

    section(g_ui.content, "NETWORKING");
    for(int i = 0; i < n; i++) {
        char key[24];
        snprintf(key, sizeof(key), "net%d", i);
        live_row(g_ui.content, st[i].name, key,
                 st[i].up ? "ONLINE" : "OFFLINE",
                 st[i].up ? PDA_CARRIER_UP : PDA_CARRIER_DOWN);
    }

    /* the one URL the nstr carrier dials, beside the state rows it explains.
       The *effective* URL is the field's text when one is configured (env
       wins over the pref - the house rule) and the example is the
       placeholder when there is none: text_field() only ever sets a
       placeholder, so a saved value has to go in the text to be visible. */
    lv_obj_t * rr = row(g_ui.content, tr(TR_RELAY));
    const char * eff = getenv("PDA_NSTR_RELAY");
    if(!eff || !*eff) eff = prefs_str("nstr_relay", "");
    lv_obj_t * rta = text_field(rr, "wss://relay.example.com");
    if(eff && *eff) lv_textarea_set_text(rta, eff);
    lv_obj_add_event_cb(rta, other_ui_on_relay, LV_EVENT_READY, NULL);
    if(g_ui.group) lv_group_add_obj(g_ui.group, rta);
    g_ui.relay_ta = rta;
    g_ui.relay_btn = save_button(rr, on_save_relay);
    g_ui.relay_btn_lbl = lv_obj_get_child(g_ui.relay_btn, 0);

    section(g_ui.content, "HARDWARE");
    live_row(g_ui.content, "WiFi", "wifi", wifi_up() ? "CONNECTED" : "OFFLINE",
             wifi_up() ? PDA_CARRIER_UP : PDA_CARRIER_DOWN);
    const gps_fix_t * g = gps_current();
    live_row(g_ui.content, "GPS", "gps",
             g->valid ? (g->simulated ? "SIMULATED" : "FIX") : "NO FIX",
             g->valid ? PDA_CARRIER_UP : PDA_CARRIER_DOWN);
    live_row(g_ui.content, "System", "system", "OK", PDA_CARRIER_UP);
}

/* the mesh: the device in the middle and the devices it has heard around
   it, the way the bitchat app draws it */
static void mesh_text(lv_layer_t * layer, const char * text, int cx, int y, uint32_t color)
{
    if(!text || !text[0]) return;
    lv_draw_label_dsc_t dsc;
    lv_draw_label_dsc_init(&dsc);
    dsc.text = text;
    dsc.font = &pda_font_mono_10;
    dsc.color = lv_color_hex(color);
    dsc.align = LV_TEXT_ALIGN_CENTER;
    lv_point_t sz;
    lv_text_get_size(&sz, text, dsc.font, 0, 0, LV_COORD_MAX, LV_TEXT_ALIGN_CENTER);
    lv_area_t area = { cx - sz.x / 2, y, cx - sz.x / 2 + sz.x, y + sz.y };
    lv_draw_label(layer, &dsc, &area);
}

static void mesh_node(lv_layer_t * layer, int x, int y, int r, uint32_t ring, uint32_t fill)
{
    lv_draw_rect_dsc_t rect;
    lv_draw_rect_dsc_init(&rect);
    rect.bg_color = lv_color_hex(fill);
    rect.bg_opa = LV_OPA_COVER;
    rect.border_color = lv_color_hex(ring);
    rect.border_width = 1;
    rect.radius = LV_RADIUS_CIRCLE;
    lv_area_t area = { x - r, y - r, x + r, y + r };
    lv_draw_rect(layer, &rect, &area);
}

static void draw_mesh(void)
{
    if(!g_ui.mesh_canvas) return;

    memset(g_ui.mesh_buf, 0, sizeof(g_ui.mesh_buf));

    int n = chat_peer_count();
    if(n > MESH_NODES) n = MESH_NODES;
    int cx = MESH_W / 2;
    int cy = MESH_H / 2 - 6;
    /* the circle has to fit: the node and the name under it as well */
    int radius = MESH_W / 2 - 90;
    if(radius > MESH_H / 2 - 50) radius = MESH_H / 2 - 50;
    if(radius < 28) radius = 28;

    lv_layer_t layer;
    lv_canvas_init_layer(g_ui.mesh_canvas, &layer);

    /* the links from the middle to the peers */
    for(int i = 0; i < n; i++) {
        const chat_peer_t * p = chat_peer_at(i);
        if(!p) continue;
        double a = -M_PI / 2.0 + (double)i * 2.0 * M_PI / (n > 1 ? n : 1);
        int x = cx + (int)(radius * cos(a));
        int y = cy + (int)(radius * sin(a));

        lv_draw_line_dsc_t line;
        lv_draw_line_dsc_init(&line);
        line.color = lv_color_hex(PDA_FRAME_INNER);
        line.width = 1;
        line.p1.x = cx;
        line.p1.y = cy;
        line.p2.x = x;
        line.p2.y = y;
        lv_draw_line(&layer, &line);

        mesh_node(&layer, x, y, 9, PDA_MSG_ACCENT, PDA_PAGE_BG);
        mesh_text(&layer, p->name, x, y + 13, PDA_MSG_TEXT);
    }

    /* this device, in the middle */
    mesh_node(&layer, cx, cy, 11, PDA_MAP_ME, PDA_MAP_ME);
    mesh_text(&layer, chat_identity(), cx, cy + 15, PDA_MSG_ACCENT);

    if(n == 0)
        mesh_text(&layer, "no peers heard yet", cx, MESH_H - 22, PDA_MSG_SYS);

    lv_canvas_finish_layer(g_ui.mesh_canvas, &layer);
}

static void page_bitchat(void)
{
    const char * sock = getenv("PDA_BITCHAT_SOCK");
    char buf[128];

    section(g_ui.content, "BITCHAT - BLUETOOTH");

    /* the mesh: the nodes that have been heard, around this device */
    lv_obj_t * canvas = lv_canvas_create(g_ui.content);
    lv_canvas_set_buffer(canvas, g_ui.mesh_buf, MESH_W, MESH_H, LV_COLOR_FORMAT_ARGB8888);
    lv_obj_set_size(canvas, MESH_W, MESH_H);
    lv_obj_set_style_min_width(canvas, MESH_W, LV_PART_MAIN);
    lv_obj_set_style_min_height(canvas, MESH_H, LV_PART_MAIN);
    lv_obj_set_flex_grow(canvas, 0);
    lv_obj_set_style_bg_opa(canvas, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(canvas, 0, LV_PART_MAIN);
    lv_obj_set_clickable(canvas, false);
    g_ui.mesh_canvas = canvas;
    draw_mesh();

    lv_obj_t * r = row(g_ui.content, "Channel");
    lv_obj_t * ta = text_field(r, chat_mesh_channel());
    lv_obj_add_event_cb(ta, other_ui_on_channel, LV_EVENT_READY, NULL);
    if(g_ui.group) lv_group_add_obj(g_ui.group, ta);
    g_ui.chan_ta = ta;
    g_ui.chan_btn = save_button(r, on_save_channel);
    g_ui.chan_btn_lbl = lv_obj_get_child(g_ui.chan_btn, 0);

    live_row(g_ui.content, "Bluetooth", "bit_radio", "no adapter", PDA_MSG_TEXT);
    /* the LAN port is BIT's: the old NET row showed PDA_NET_PORT (7788),
       and that half of NET left with spec-nstr-relay B1 */
    snprintf(buf, sizeof(buf), "%s", getenv("PDA_BITCHAT_PORT") ? getenv("PDA_BITCHAT_PORT")
                                                                : "2425");
    live_row(g_ui.content, "LAN port", "net_port", buf, PDA_MSG_TEXT);
    live_row(g_ui.content, "Discovery", "bit_scan", "off", PDA_MSG_TEXT);
    live_row(g_ui.content, "Bridge", "bit_bridge",
             (sock && *sock) ? sock : "not connected",
             (sock && *sock) ? PDA_CARRIER_UP : PDA_CARRIER_DOWN);
    live_row(g_ui.content, "Node name", "bit_name", chat_identity(), PDA_MSG_TEXT);

    section(g_ui.content, "LINK");
    live_row(g_ui.content, "State", "bit_state", "offline", PDA_CARRIER_DOWN);
    live_row(g_ui.content, "Peers heard", "bit_peers", "0", PDA_MSG_TEXT);

    /* the mesh needs the port open: a firewall that drops what comes in
       leaves the device deaf, and nobody can hear it either */
    lv_obj_t * hint = label(g_ui.content, 0, 0, 300,
                             "no peers? the firewall may hold port 2425\n"
                             "sudo ufw allow 2425/udp",
                             &pda_font_mono_10, PDA_MSG_SYS);
    lv_obj_set_flex_grow(hint, 0);
    lv_obj_set_height(hint, 26);
    g_ui.bit_hint = hint;
}

static void page_fips(void)
{
    char buf[160];

    section(g_ui.content, "F.I.P.S - MESH");
    snprintf(buf, sizeof(buf), "%s", getenv("PDA_FIPS_SOCK") ? getenv("PDA_FIPS_SOCK")
                                                              : "/run/fips/api.sock");
    live_row(g_ui.content, "API socket", "fips_api", buf, PDA_MSG_TEXT);
    snprintf(buf, sizeof(buf), "%s", getenv("PDA_FIPS_CTL") ? getenv("PDA_FIPS_CTL")
                                                              : "/run/fips/control.sock");
    live_row(g_ui.content, "Control", "fips_ctl", buf, PDA_MSG_TEXT);
    snprintf(buf, sizeof(buf), "%s", getenv("PDA_FIPS_PORT") ? getenv("PDA_FIPS_PORT")
                                                             : "4600");
    live_row(g_ui.content, "FSP port", "fips_port", buf, PDA_MSG_TEXT);
    live_row(g_ui.content, "Mode", "fips_mode", "leaf only", PDA_MSG_TEXT);

    section(g_ui.content, "LINK");
    live_row(g_ui.content, "State", "fips_state", "offline", PDA_CARRIER_DOWN);
    live_row(g_ui.content, "Peers", "fips_peers", "-", PDA_MSG_TEXT);
    live_row(g_ui.content, "RTT / loss", "fips_mmp", "-", PDA_MSG_TEXT);
}

static void page_wifi(void)
{
    char name[64] = "-", ip[64] = "-", mac[64] = "-";

    section(g_ui.content, "WIFI");
    wifi_info(name, sizeof(name), ip, sizeof(ip), mac, sizeof(mac));
    live_row(g_ui.content, "Interface", "wifi_if", name, PDA_MSG_TEXT);
    live_row(g_ui.content, "Address", "wifi_ip", ip, PDA_MSG_TEXT);
    live_row(g_ui.content, "MAC", "wifi_mac", mac, PDA_MSG_TEXT);
    live_row(g_ui.content, "State", "wifi_state", wifi_up() ? "CONNECTED" : "OFFLINE",
             wifi_up() ? PDA_CARRIER_UP : PDA_CARRIER_DOWN);
    live_row(g_ui.content, "Mode", "wifi_mode", "client", PDA_MSG_TEXT);
}

static void page_gps(void)
{
    const gps_fix_t * g = gps_current();
    char buf[64];

    section(g_ui.content, "GPS");
    fmt_coord(g->lat, buf, sizeof(buf));
    live_row(g_ui.content, "Latitude", "gps_lat", buf, PDA_MSG_TEXT);
    fmt_coord(g->lon, buf, sizeof(buf));
    live_row(g_ui.content, "Longitude", "gps_lon", buf, PDA_MSG_TEXT);
    snprintf(buf, sizeof(buf), "%d", g->sats);
    live_row(g_ui.content, "Satellites", "gps_sats", buf, PDA_MSG_TEXT);
    fmt_speed(g->speed, buf, sizeof(buf));
    live_row(g_ui.content, "Speed", "gps_speed", buf, PDA_MSG_TEXT);
    live_row(g_ui.content, "Fix", "gps_fix",
             g->valid ? (g->simulated ? "SIMULATED" : "FIX") : "NO FIX",
             g->valid ? PDA_CARRIER_UP : PDA_CARRIER_DOWN);
    live_row(g_ui.content, "Source", "gps_src", gps_daemon_seen() ? "gpsd" : "none",
             gps_daemon_seen() ? PDA_CARRIER_UP : PDA_CARRIER_DOWN);
}

static void page_system(void)
{
    section(g_ui.content, "SYSTEM");
    live_row(g_ui.content, "Version", "sys_version", s_version, PDA_MSG_TEXT);
    live_row(g_ui.content, "GPS", "sys_gps", s_gps, PDA_MSG_TEXT);
    live_row(g_ui.content, "Screen", "sys_screen", "800 x 480", PDA_MSG_TEXT);
    live_row(g_ui.content, "Carriers", "sys_carriers",
             chat_transport_count() == 1 ? "1" : (chat_transport_count() == 2 ? "2" :
             (chat_transport_count() == 3 ? "3" : "?")), PDA_MSG_TEXT);
    live_row(g_ui.content, "Language", "sys_lang", tr_lang_name(tr_lang()), PDA_MSG_TEXT);
    live_row(g_ui.content, "Theme", "sys_theme", pda_scheme_name(pda_theme_scheme()),
             PDA_MSG_TEXT);
    live_row(g_ui.content, "Settings file", "sys_prefs", prefs_path(), PDA_MSG_TEXT);
}

/* ---------- the settings that do something ---------- */

/* a setting with a few choices: the value cycles when it is clicked */
typedef void (*apply_fn)(const char * key);

typedef struct {
    const char * key;
    const char * const * values;      /* what is saved */
    const char * const * labels;      /* what is shown */
    int n;
    int index;
    lv_obj_t * lbl;
    apply_fn apply;
} cycle_t;

#define CYCLE_MAX 8
static cycle_t g_cycle[CYCLE_MAX];
static int g_n_cycle;

static const char * const v_time[]   = { "24", "12" };
static const char * const l_time[]   = { "24 hour", "12 hour" };
static const char * const v_units[]  = { "metric", "imperial" };
static const char * const l_units[]  = { "Metric (km/h)", "Imperial (mph)" };
static const char * const v_start[]  = { "0", "1", "2", "3", "4", "5", "6" };
static const char * const l_start[]  = { "Area map", "Messages", "Contacts", "Radio",
                                         "Camera", "Wallet", "Other" };
static const char * const v_gps[]    = { "1000", "2000", "5000", "10000" };
static const char * const l_gps[]    = { "1 s", "2 s", "5 s", "10 s" };
static const char * const v_log[]    = { "200", "500", "1000" };
static const char * const l_log[]    = { "200 ms", "500 ms", "1 s" };
static const char * const v_lang[]   = { "en", "pl" };
static const char * const l_lang[]   = { "English", "Polski" };
static const char * const v_tz[]     = { "local", "utc", "lisbon" };
static const char * const l_tz[]     = { "Local", "UTC", "Lisbon" };
static const char * const v_region[] = { "dot", "comma" };
static const char * const l_region[] = { "1.234.5", "1.234,5" };
static const char * const v_scheme[] = { "wasteland", "ash", "rust" };
static const char * const l_scheme[] = { "Wasteland", "Ash", "Rust" };
static const char * const v_accent[] = { "amber", "green", "blue" };
static const char * const l_accent[] = { "Amber", "Green", "Blue" };
static const char * const v_grain[]  = { "on", "off" };
static const char * const l_grain[]  = { "On", "Off" };

/* the panels are rebuilt when the theme or the language changes - but not
   from inside the click handler: the button that was clicked is part of the
   tree that goes away, so the work waits for the next round */
static void retheme_async(void * arg)
{
    LV_UNUSED(arg);
    pda_shell_retheme();
}

static void apply_theme(const char * key)
{
    LV_UNUSED(key);
    pda_theme_load();
    lv_async_call(retheme_async, NULL);
}

static void apply_language(const char * key)
{
    LV_UNUSED(key);
    tr_load();
    lv_async_call(retheme_async, NULL);
}

static void apply_gps_poll(const char * key)
{
    LV_UNUSED(key);
    pda_shell_set_gps_poll(atoi(prefs_str("gps_poll", "1000")));
}

static void apply_log_poll(const char * key)
{
    LV_UNUSED(key);
    chat_ui_set_poll(atoi(prefs_str("log_poll", "200")));
}

static void apply_reset_log(const char * key)
{
    LV_UNUSED(key);
    chat_ui_clear();
}

/* the clock, the units, the region and the time zone all live in the
   format module */
static void apply_format(const char * key)
{
    LV_UNUSED(key);
    fmt_apply();
    other_ui_refresh();
}

static void apply_grain(const char * key)
{
    LV_UNUSED(key);
    grit_set_grain(!strcmp(prefs_str("grain", "on"), "on"));
}

static void on_cycle_click(lv_event_t * e)
{
    int i = (int)(lv_intptr_t)lv_event_get_user_data(e);
    if(i < 0 || i >= g_n_cycle) return;
    cycle_t * c = &g_cycle[i];
    c->index = (c->index + 1) % c->n;
    prefs_set_str(c->key, c->values[c->index]);
    if(c->lbl) lv_label_set_text(c->lbl, c->labels[c->index]);
    if(c->apply) c->apply(c->key);
}

/* a row whose value steps through the choices when it is clicked */
static lv_obj_t * cycle_row(lv_obj_t * parent, const char * name, const char * key,
                            const char * const * values, const char * const * labels,
                            int n, apply_fn apply)
{
    lv_obj_t * r = row(parent, name);
    if(g_n_cycle >= CYCLE_MAX) return r;

    const char * cur = prefs_str(key, values[0]);
    int idx = 0;
    for(int i = 0; i < n; i++) if(!strcmp(cur, values[i])) idx = i;

    lv_obj_t * btn = lv_button_create(r);
    lv_obj_set_pos(btn, VAL_X, 0);
    lv_obj_set_size(btn, 290, 17);
    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, LV_STATE_PRESSED);
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_outline_width(btn, 0, LV_PART_MAIN);
    lv_no_transition(btn);

    lv_obj_t * lbl = label(btn, VAL_PAD, VAL_Y, 0, labels[idx], &pda_font_mono_10, PDA_MSG_TEXT);
    lv_obj_set_style_text_color(lbl, lv_color_hex(PDA_MSG_ACCENT), LV_PART_MAIN);

    cycle_t * c = &g_cycle[g_n_cycle];
    c->key = key;
    c->values = values;
    c->labels = labels;
    c->n = n;
    c->index = idx;
    c->lbl = lbl;
    c->apply = apply;
    lv_obj_add_event_cb(btn, on_cycle_click, LV_EVENT_CLICKED,
                        (void *)(lv_intptr_t)g_n_cycle);
    g_n_cycle++;
    return r;
}

/* a row with a button that does something */
static lv_obj_t * action_row(lv_obj_t * parent, const char * name, const char * text,
                             lv_event_cb_t cb)
{
    lv_obj_t * r = row(parent, name);
    lv_obj_t * b = lv_button_create(r);
    lv_obj_set_pos(b, VAL_X, 0);
    lv_obj_set_size(b, 52, 17);
    lv_obj_set_style_bg_color(b, lv_color_hex(PDA_CHAT_EDGE), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(b, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(b, lv_color_hex(PDA_MSG_ACCENT), LV_PART_MAIN);
    lv_obj_set_style_radius(b, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(b, 0, LV_PART_MAIN);
    lv_no_transition(b);
    lv_obj_t * l = lv_label_create(b);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_font(l, &pda_font_mono_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(l, lv_color_hex(PDA_MSG_ACCENT), LV_PART_MAIN);
    lv_obj_center(l);
    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, NULL);
    return r;
}

static void page_general(void)
{
    section(g_ui.content, "GENERAL");

    /* the name the device answers to - the same one the messages carry */
    lv_obj_t * r = row(g_ui.content, "Device name");
    lv_obj_t * ta = text_field(r, chat_identity());
    lv_obj_add_event_cb(ta, other_ui_on_name, LV_EVENT_READY, NULL);
    if(g_ui.group) lv_group_add_obj(g_ui.group, ta);
    g_ui.name_ta = ta;
    g_ui.name_btn = save_button(r, on_save_name);
    g_ui.name_btn_lbl = lv_obj_get_child(g_ui.name_btn, 0);

    cycle_row(g_ui.content, "Time format", "time_fmt", v_time, l_time, 2, apply_format);
    cycle_row(g_ui.content, "Units", "units", v_units, l_units, 2, apply_format);
    cycle_row(g_ui.content, "Startup screen", "start_tab", v_start, l_start, 7, NULL);
}

static void page_advanced(void)
{
    section(g_ui.content, "ADVANCED");
    cycle_row(g_ui.content, "GPS refresh", "gps_poll", v_gps, l_gps, 4, apply_gps_poll);
    cycle_row(g_ui.content, "Log refresh", "log_poll", v_log, l_log, 3, apply_log_poll);
    action_row(g_ui.content, "Reset log", "CLEAR", on_reset_log);
}

static void page_language(void)
{
    section(g_ui.content, "LANGUAGE");
    cycle_row(g_ui.content, "Language", "lang", v_lang, l_lang, 2, apply_language);
    cycle_row(g_ui.content, "Time zone", "tz", v_tz, l_tz, 3, apply_format);
    cycle_row(g_ui.content, "Region", "region", v_region, l_region, 2, apply_format);
}

static void page_themes(void)
{
    section(g_ui.content, "THEMES");
    cycle_row(g_ui.content, "Colour scheme", "scheme", v_scheme, l_scheme, 3, apply_theme);
    cycle_row(g_ui.content, "Accent colour", "accent", v_accent, l_accent, 3, apply_theme);
    cycle_row(g_ui.content, "Grain", "grain", v_grain, l_grain, 2, apply_grain);
}

static void page_settings(int sub)
{
    switch(sub) {
        case 0: page_general(); break;
        case 1: page_advanced(); break;
        case 2: page_language(); break;
        case 3: page_themes(); break;
        case 4: page_system(); break;
        default: page_user(); break;
    }
}

/* the little SAVE button next to a text field */
static lv_obj_t * save_button(lv_obj_t * row_obj, lv_event_cb_t cb)
{
    lv_obj_t * b = lv_button_create(row_obj);
    lv_obj_set_pos(b, VAL_X + 232, 0);
    lv_obj_set_size(b, 52, 17);
    lv_obj_set_style_bg_color(b, lv_color_hex(PDA_CHAT_EDGE), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(b, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(b, lv_color_hex(PDA_MSG_ACCENT), LV_PART_MAIN);
    lv_obj_set_style_radius(b, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(b, 0, LV_PART_MAIN);
    lv_no_transition(b);
    lv_obj_t * l = lv_label_create(b);
    lv_label_set_text(l, tr(TR_SAVE));
    lv_obj_set_style_text_font(l, &pda_font_mono_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(l, lv_color_hex(PDA_MSG_ACCENT), LV_PART_MAIN);
    lv_obj_center(l);
    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, NULL);
    return b;
}

/* the pending "SAVED" flash: the label it points at is deleted with the
   page, so the timer has to go before the page is rebuilt */
static lv_timer_t * s_flash_timer = NULL;

static void saved_flash_cb(lv_timer_t * timer)
{
    lv_obj_t * l = (lv_obj_t *)lv_timer_get_user_data(timer);
    if(l) {
        lv_label_set_text(l, tr(TR_SAVE));
        lv_obj_set_style_text_color(l, lv_color_hex(PDA_MSG_ACCENT), LV_PART_MAIN);
    }
    s_flash_timer = NULL;      /* the timer deletes itself: say so */
    lv_timer_delete(timer);
}

static void flash_saved(lv_obj_t * btn, lv_obj_t * lbl)
{
    if(!btn || !lbl) return;
    lv_label_set_text(lbl, tr(TR_SAVED));
    lv_obj_set_style_text_color(lbl, lv_color_hex(PDA_CARRIER_UP), LV_PART_MAIN);
    if(s_flash_timer) {
        lv_timer_t * old = s_flash_timer;
        s_flash_timer = NULL;
        lv_timer_delete(old);
    }
    s_flash_timer = lv_timer_create(saved_flash_cb, 1500, NULL);
    lv_timer_set_user_data(s_flash_timer, lbl);
    lv_timer_resume(s_flash_timer);
}

/* The flash on [KEY]/[NEW]/[PASTE]: same single 1500 ms timer as
   flash_saved, but it restores *this button's* label (flash_saved is for
   SAVE buttons only and must not be reused here). Its timer is registered
   with build_content()'s flash cleanup exactly like s_flash_timer, so a
   rebuild during a pending flash never leaves a timer pointing at a
   deleted label. */
static lv_timer_t * s_key_flash_timer = NULL;
static char s_key_flash_restore[16];

static void key_flash_cb(lv_timer_t * timer)
{
    lv_obj_t * l = (lv_obj_t *)lv_timer_get_user_data(timer);
    if(l) {
        lv_label_set_text(l, s_key_flash_restore);
        lv_obj_set_style_text_color(l, lv_color_hex(PDA_MSG_ACCENT), LV_PART_MAIN);
    }
    s_key_flash_timer = NULL;      /* the timer deletes itself: say so */
    lv_timer_delete(timer);
}

/* shown = what the label says for the flash, restore = its own text after
   1500 ms, color = green for a success, the failure colour for FAILED */
static void flash_btn(lv_obj_t * lbl, const char * shown, const char * restore,
                      uint32_t color)
{
    if(!lbl) return;
    lv_label_set_text(lbl, shown);
    lv_obj_set_style_text_color(lbl, lv_color_hex(color), LV_PART_MAIN);
    snprintf(s_key_flash_restore, sizeof(s_key_flash_restore), "%s", restore);
    if(s_key_flash_timer) {
        lv_timer_t * old = s_key_flash_timer;
        s_key_flash_timer = NULL;
        lv_timer_delete(old);
    }
    s_key_flash_timer = lv_timer_create(key_flash_cb, 1500, NULL);
    lv_timer_set_user_data(s_key_flash_timer, lbl);
}

/* a row with a switch on the right */
static void toggle_row(lv_obj_t * parent, const char * name, const char * key)
{
    lv_obj_t * r = row(parent, name);
    bool on = prefs_bool(key, true);
    lv_obj_t * b = toggle(r, on);
    lv_obj_set_pos(b, VAL_X, 1);
    lv_obj_add_event_cb(b, on_toggle_click, LV_EVENT_CLICKED,
                        (void *)(lv_intptr_t)(g_ui.n_tog));
    if(g_ui.n_tog < 4) {
        g_ui.tog[g_ui.n_tog] = b;
        snprintf(g_ui.tog_key[g_ui.n_tog], sizeof(g_ui.tog_key[0]), "%s", key);
        g_ui.n_tog++;
    }
}

/* one accent button, styled like action_row's (action_row hardcodes its
   label into a row of its own, and these live in rows with other things) */
static lv_obj_t * key_button(lv_obj_t * r, int x, const char * text,
                             lv_event_cb_t cb)
{
    lv_obj_t * b = lv_button_create(r);
    lv_obj_set_pos(b, x, 0);
    lv_obj_set_size(b, 52, 17);
    lv_obj_set_style_bg_color(b, lv_color_hex(PDA_CHAT_EDGE), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(b, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(b, lv_color_hex(PDA_MSG_ACCENT), LV_PART_MAIN);
    lv_obj_set_style_radius(b, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(b, 0, LV_PART_MAIN);
    lv_no_transition(b);
    lv_obj_t * l = lv_label_create(b);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_font(l, &pda_font_mono_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(l, lv_color_hex(PDA_MSG_ACCENT), LV_PART_MAIN);
    lv_obj_center(l);
    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, NULL);
    return b;
}

/* the [KEY] block: the full npub on one line, the full nsec on the next,
   spanning the row width (the value column alone would cut either of them
   short). It exists only while s_show_key says so. */
static void key_block(void)
{
    lv_obj_t * b = plain(g_ui.content, lv_pct(100), 34);
    lv_obj_set_flex_grow(b, 0);
    const char * pub = identity_pub_hex();
    const char * npub = pub[0] ? identity_npub() : tr(TR_KEY_NOT_SET);
    const char * nsec = pub[0] ? identity_nsec() : tr(TR_KEY_NOT_SET);
    label(b, VAL_PAD, 1, 454, npub, &pda_font_mono_10, PDA_MSG_TEXT);
    label(b, VAL_PAD, 17, 454, nsec, &pda_font_mono_10, PDA_MSG_SYS);
}

static void page_user(void)
{
    section(g_ui.content, "IDENTITY");

    lv_obj_t * r = row(g_ui.content, "Name");
    lv_obj_t * ta = text_field(r, chat_identity());
    lv_obj_add_event_cb(ta, other_ui_on_name, LV_EVENT_READY, NULL);
    if(g_ui.group) lv_group_add_obj(g_ui.group, ta);
    g_ui.name_ta = ta;
    g_ui.name_btn = save_button(r, on_save_name);
    g_ui.name_btn_lbl = lv_obj_get_child(g_ui.name_btn, 0);

    lv_obj_t * r2 = row(g_ui.content, "Room");
    lv_obj_t * ta2 = text_field(r2, chat_room());
    lv_obj_add_event_cb(ta2, other_ui_on_room, LV_EVENT_READY, NULL);
    if(g_ui.group) lv_group_add_obj(g_ui.group, ta2);
    g_ui.room_ta = ta2;
    g_ui.room_btn = save_button(r2, on_save_room);
    g_ui.room_btn_lbl = lv_obj_get_child(g_ui.room_btn, 0);

    /* the device's nstr key: the key underneath the name. The name stays
       a label - it just stops being the identity */
    lv_obj_t * rk = row(g_ui.content, tr(TR_KEY_ROW));
    const char * pub = identity_pub_hex();
    g_ui.key_btn_lbl = NULL;
    if(pub[0]) {
        label(rk, VAL_X + VAL_PAD, VAL_Y, 220, identity_npub(),
              &pda_font_mono_10, PDA_MSG_TEXT);
    }
    else {
        label(rk, VAL_X + VAL_PAD, VAL_Y, 220, tr(TR_KEY_NOT_SET),
              &pda_font_mono_10, PDA_MSG_SYS);
    }
    lv_obj_t * kb = key_button(rk, VAL_X + 232, tr(TR_KEY_KEY), on_key_toggle);
    g_ui.key_btn_lbl = lv_obj_get_child(kb, 0);
    if(g_ui.group) lv_group_add_obj(g_ui.group, kb);

    /* the reveal: up right after the press, gone on the next rebuild */
    if(s_show_key) key_block();

    /* [NEW] behind a two-press confirm - your old pubkey becomes a
       stranger to everyone who Trusted it until they do again */
    lv_obj_t * nr = row(g_ui.content, "");
    lv_obj_t * nb = key_button(nr, VAL_X, tr(TR_KEY_NEW), on_new_click);
    g_ui.new_btn_lbl = lv_obj_get_child(nb, 0);
    if(g_ui.group) lv_group_add_obj(g_ui.group, nb);

    /* [PASTE] - an nsec (bech32 or 64 hex) validated before anything is
       saved; invalid input is an honest error and saves nothing */
    lv_obj_t * pr = row(g_ui.content, "");
    lv_obj_t * pta = text_field(pr, tr(TR_KEY_PASTE_HINT));
    lv_obj_add_event_cb(pta, other_ui_on_paste, LV_EVENT_READY, NULL);
    if(g_ui.group) lv_group_add_obj(g_ui.group, pta);
    g_ui.paste_ta = pta;
    lv_obj_t * pb = key_button(pr, VAL_X + 232, tr(TR_KEY_PASTE), on_paste_click);
    g_ui.paste_btn_lbl = lv_obj_get_child(pb, 0);
    if(g_ui.group) lv_group_add_obj(g_ui.group, pb);

    section(g_ui.content, "MESSAGES");
    toggle_row(g_ui.content, "Show carrier tag", "show_carrier");
    toggle_row(g_ui.content, "Show timestamps", "show_time");
    toggle_row(g_ui.content, "Show system lines", "show_system");
}

static void page_extras(void)
{
    lv_obj_t * l = label(g_ui.content, 0, 0, 0, "nothing here yet", &pda_font_mono_10,
                         PDA_MSG_SYS);
    lv_obj_set_height(l, 20);
}

/* ---------- the pages are built here ---------- */

static void build_content(void)
{
    /* the flash timer points at a label that is about to be deleted */
    if(s_flash_timer) {
        lv_timer_t * old = s_flash_timer;
        s_flash_timer = NULL;
        lv_timer_delete(old);
    }
    /* and so does any transition the theme started on a button that is about
       to go away: an animation outlives its object otherwise */
    lv_anim_delete_all();
    lv_kill_tree(g_ui.content);
    lv_obj_clean(g_ui.content);
    g_ui.n_live = 0;
    g_ui.n_tog = 0;
    g_ui.sec_no = 0;
    g_n_cycle = 0;
    /* the key flash timer points at a label that is about to be deleted */
    if(s_key_flash_timer) {
        lv_timer_t * old = s_key_flash_timer;
        s_key_flash_timer = NULL;
        lv_timer_delete(old);
    }
    s_new_armed = false;         /* a rebuilt page starts with "NEW", not
                                    a half-armed confirm */
    g_ui.name_ta = NULL;
    g_ui.room_ta = NULL;
    g_ui.chan_ta = NULL;
    g_ui.relay_ta = NULL;
    g_ui.relay_btn = NULL;
    g_ui.relay_btn_lbl = NULL;
    g_ui.mesh_canvas = NULL;
    g_ui.bit_hint = NULL;
    g_ui.key_btn_lbl = NULL;
    g_ui.new_btn_lbl = NULL;
    g_ui.paste_btn_lbl = NULL;
    g_ui.paste_ta = NULL;

    if(g_ui.page == PAGE_SETTINGS) page_settings(g_ui.sub);
    else if(g_ui.page == PAGE_CONNECTIONS) {
        switch(g_ui.sub) {
            case 0: page_info(); break;
            case 1: page_bitchat(); break;
            case 2: page_fips(); break;
            case 3: page_wifi(); break;
            default: page_gps(); break;
        }
    }
    else page_extras();

    other_ui_refresh();
    if(getenv("PDA_DEBUG_FIELD")) {
        uint32_t cnt = lv_obj_get_child_count(g_ui.content);
        for(uint32_t i = 0; i < cnt; i++) {
            lv_obj_t * rp = lv_obj_get_child(g_ui.content, i);
            uint32_t rc = lv_obj_get_child_count(rp);
            for(uint32_t j = 0; j < rc; j++) {
                lv_obj_t * o = lv_obj_get_child(rp, j);
                if(!lv_obj_check_type(o, &lv_textarea_class)) continue;
                lv_obj_update_layout(o);
                lv_area_t a;
                lv_obj_get_coords(o, &a);        /* absolute = window client */
                fprintf(stderr, "[field] built: x=%d y=%d w=%d h=%d content=%d "
                        "textlen=%d placeholder=%s\n",
                        a.x1, a.y1, lv_obj_get_width(o), lv_obj_get_height(o),
                        lv_obj_get_content_width(o),
                        (int)strlen(lv_textarea_get_text(o)),
                        lv_textarea_get_placeholder_text(o));
            }
        }
    }
}

/* ---------- the live values ---------- */

static void set_live(int i, const char * text, uint32_t color)
{
    if(i >= g_ui.n_live || !g_ui.live[i]) return;
    lv_obj_t * o = g_ui.live[i];
    if(lv_obj_check_type(o, &lv_textarea_class)) lv_textarea_set_text(o, text);
    else lv_label_set_text(o, text);
    lv_obj_set_style_text_color(o, lv_color_hex(color), LV_PART_MAIN);
}

void other_ui_refresh(void)
{
    const char * key;
    for(int i = 0; i < g_ui.n_live; i++) {
        key = g_ui.live_key[i];
        if(!strcmp(key, "net0") || !strcmp(key, "net1") || !strcmp(key, "net2")) {
            int idx = key[3] - '0';
            net_state_t st[8];
            int n = collect_states(st, 8);
            if(idx < n) {
                const chat_transport_t * tr = chat_transport_get(idx);
                set_live(i, tr ? chat_carrier_state_name(tr->state) : "OFFLINE",
                         tr ? lv_carrier_color(tr->state) : PDA_CARRIER_DOWN);
            }
        }
        else if(!strcmp(key, "bit_peers")) {
            char n[16];
            snprintf(n, sizeof(n), "%d", chat_peer_count());
            set_live(i, n, PDA_MSG_TEXT);
            /* the hint is only worth the room when nobody answers */
            if(g_ui.bit_hint) {
                if(chat_peer_count() > 0) lv_obj_set_hidden(g_ui.bit_hint, true);
                else lv_obj_set_hidden(g_ui.bit_hint, false);
            }
        }
        else if(!strcmp(key, "bit_scan")) {
            char scan[96] = "off";
            bit_scan_text(scan, sizeof(scan));
            set_live(i, scan, PDA_MSG_TEXT);
        }
        else if(!strcmp(key, "bit_radio")) {
            char radio[64] = "no adapter";
            bit_info_text(radio, sizeof(radio));
            set_live(i, radio, PDA_MSG_TEXT);
        }
        else if(!strcmp(key, "bit_state")) {
            const chat_transport_t * tr = NULL;
            for(int k = 0; k < chat_transport_count(); k++) {
                const chat_transport_t * t = chat_transport_get(k);
                if(t && !strcmp(t->tag, "BIT")) tr = t;
            }
            set_live(i, tr ? chat_carrier_state_name(tr->state) : "OFFLINE",
                     tr ? lv_carrier_color(tr->state) : PDA_CARRIER_DOWN);
        }
        else if(!strcmp(key, "fips_state")) {
            const chat_transport_t * tr = NULL;
            for(int k = 0; k < chat_transport_count(); k++) {
                const chat_transport_t * t = chat_transport_get(k);
                if(t && !strcmp(t->tag, "FIPS")) tr = t;
            }
            set_live(i, tr ? chat_carrier_state_name(tr->state) : "OFFLINE",
                     tr ? lv_carrier_color(tr->state) : PDA_CARRIER_DOWN);
        }
        else if(!strcmp(key, "wifi")) {
            bool up = wifi_up();
            set_live(i, up ? "CONNECTED" : "OFFLINE", up ? PDA_CARRIER_UP : PDA_CARRIER_DOWN);
        }
        else if(!strcmp(key, "wifi_state")) {
            bool up = wifi_up();
            set_live(i, up ? "CONNECTED" : "OFFLINE", up ? PDA_CARRIER_UP : PDA_CARRIER_DOWN);
        }
        else if(!strcmp(key, "gps") || !strcmp(key, "gps_fix")) {
            const gps_fix_t * g = gps_current();
            set_live(i, g->valid ? (g->simulated ? "SIMULATED" : "FIX") : "NO FIX",
                     g->valid ? PDA_CARRIER_UP : PDA_CARRIER_DOWN);
        }
        else if(!strcmp(key, "gps_lat")) {
            char b[32];
            fmt_coord(gps_current()->lat, b, sizeof(b));
            set_live(i, b, PDA_MSG_TEXT);
        }
        else if(!strcmp(key, "gps_lon")) {
            char b[32];
            fmt_coord(gps_current()->lon, b, sizeof(b));
            set_live(i, b, PDA_MSG_TEXT);
        }
        else if(!strcmp(key, "gps_sats")) {
            char b[32];
            snprintf(b, sizeof(b), "%d", gps_current()->sats);
            set_live(i, b, PDA_MSG_TEXT);
        }
        else if(!strcmp(key, "gps_speed")) {
            char b[32];
            fmt_speed(gps_current()->speed, b, sizeof(b));
            set_live(i, b, PDA_MSG_TEXT);
        }
        else if(!strcmp(key, "gps_src")) {
            bool seen = gps_daemon_seen();
            set_live(i, seen ? "gpsd" : "none", seen ? PDA_CARRIER_UP : PDA_CARRIER_DOWN);
        }
        else if(!strcmp(key, "fips_peers")) {
            char b[24];
            int n = chat_fips_peer_count();
            if(n > 0) snprintf(b, sizeof(b), "%d", n);
            else snprintf(b, sizeof(b), "-");
            set_live(i, b, n > 0 ? PDA_MSG_TEXT : PDA_MSG_SYS);
        }
        else if(!strcmp(key, "sys_version")) set_live(i, s_version, PDA_MSG_TEXT);
        else if(!strcmp(key, "sys_gps")) set_live(i, s_gps, PDA_MSG_TEXT);
    }
}

/* ---------- navigation ---------- */

static void select_page(int page);
static void select_sub(int sub);

static void on_nav_click(lv_event_t * e)
{
    int i = (int)(lv_intptr_t)lv_event_get_user_data(e);
    if(getenv("PDA_DEBUG")) fprintf(stderr, "[other] nav click %d\n", i);
    select_page(i);
}

static void on_sub_click(lv_event_t * e)
{
    int i = (int)(lv_intptr_t)lv_event_get_user_data(e);
    if(getenv("PDA_DEBUG")) fprintf(stderr, "[other] sub click %d\n", i);
    select_sub(i);
}

/* the theme's focus ring would fight with the selected state, so the
   focused state is painted like the normal one */
static void restyle_nav(void)
{
    for(int i = 0; i < PAGE_COUNT; i++) {
        bool on = (i == g_ui.page);
        uint32_t bg = on ? PDA_SEL_BAR : PDA_BG;
        lv_state_t st;
        for(int k = 0; k < 2; k++) {
            st = k ? LV_STATE_FOCUSED : LV_PART_MAIN;
            lv_obj_set_style_bg_color(g_ui.nav_box[i], lv_color_hex(bg), st);
            lv_obj_set_style_bg_opa(g_ui.nav_box[i], on ? LV_OPA_COVER : LV_OPA_TRANSP, st);
            lv_obj_set_style_border_side(g_ui.nav_box[i],
                                          on ? LV_BORDER_SIDE_LEFT : LV_BORDER_SIDE_NONE,
                                          st);
            lv_obj_set_style_border_color(g_ui.nav_box[i], lv_color_hex(PDA_MSG_ACCENT),
                                          st);
            lv_obj_set_style_border_width(g_ui.nav_box[i], on ? 2 : 0, st);
            lv_obj_set_style_outline_width(g_ui.nav_box[i], 0, st);
            lv_obj_set_style_shadow_width(g_ui.nav_box[i], 0, st);
            lv_obj_set_style_bg_color(g_ui.nav_box[i], lv_color_hex(bg), LV_STATE_PRESSED);
        }
        lv_obj_set_style_text_color(g_ui.nav_lbl[i],
                                    lv_color_hex(on ? PDA_SEL_TEXT : PDA_TAB_TEXT),
                                    LV_PART_MAIN);
    }
}

static void restyle_subs(void)
{
    for(int i = 0; i < g_ui.n_subs; i++) {
        bool on = (i == g_ui.sub);
        uint32_t bg = on ? PDA_SEL_BAR : PDA_BG;
        lv_state_t st;
        for(int k = 0; k < 2; k++) {
            st = k ? LV_STATE_FOCUSED : LV_PART_MAIN;
            lv_obj_set_style_bg_color(g_ui.sub_btn[i], lv_color_hex(bg), st);
            lv_obj_set_style_bg_opa(g_ui.sub_btn[i], on ? LV_OPA_COVER : LV_OPA_TRANSP, st);
            lv_obj_set_style_border_side(g_ui.sub_btn[i], LV_BORDER_SIDE_LEFT, st);
            lv_obj_set_style_border_color(g_ui.sub_btn[i],
                                          lv_color_hex(on ? PDA_MSG_ACCENT : PDA_CHAT_EDGE),
                                          st);
            lv_obj_set_style_border_width(g_ui.sub_btn[i], on ? 2 : 0, st);
            lv_obj_set_style_outline_width(g_ui.sub_btn[i], 0, st);
            lv_obj_set_style_shadow_width(g_ui.sub_btn[i], 0, st);
            lv_obj_set_style_bg_color(g_ui.sub_btn[i], lv_color_hex(bg), LV_STATE_PRESSED);
        }
        lv_obj_set_style_text_color(g_ui.sub_lbl[i],
                                    lv_color_hex(on ? PDA_SEL_TEXT : PDA_TAB_TEXT),
                                    LV_PART_MAIN);
    }
}

static void select_page(int page)
{
    if(page < 0 || page >= PAGE_COUNT) return;
    s_show_key = false;          /* navigation clears the reveal */
    g_ui.page = page;
    g_ui.sub = 0;
    int n = 0;
    while(n < 8 && g_sub_ids[page][n]) n++;
    g_ui.n_subs = n;
    for(int i = 0; i < 8; i++) {
        if(!g_ui.sub_btn[i]) continue;
        bool show = (i < n);
        lv_obj_set_hidden(g_ui.sub_btn[i], !show);
        if(show) lv_label_set_text(g_ui.sub_lbl[i], tr(g_sub_ids[page][i]));
    }
    if(g_ui.strip_title) lv_label_set_text(g_ui.strip_title, tr(g_page_ids[page]));
    restyle_nav();
    restyle_subs();
    build_content();
}

static void select_sub(int sub)
{
    if(sub < 0 || sub >= g_ui.n_subs) return;
    s_show_key = false;          /* navigation clears the reveal */
    g_ui.sub = sub;
    restyle_subs();
    build_content();
}

static void on_toggle_click(lv_event_t * e)
{
    int i = (int)(lv_intptr_t)lv_event_get_user_data(e);
    if(i < 0 || i >= g_ui.n_tog) return;
    bool on = !prefs_bool(g_ui.tog_key[i], true);
    prefs_set_bool(g_ui.tog_key[i], on);
    lv_obj_set_style_bg_color(g_ui.tog[i],
                              lv_color_hex(on ? PDA_MSG_ACCENT : PDA_CHAT_EDGE),
                              LV_PART_MAIN);
}

/* empty the message log */
static void on_reset_log(lv_event_t * e)
{
    LV_UNUSED(e);
    chat_ui_clear();
}

static void other_ui_tick(lv_timer_t * timer)
{
    LV_UNUSED(timer);
    other_ui_refresh();
    /* the mesh moves: new peers come and go */
    if(g_ui.page == PAGE_CONNECTIONS && g_ui.sub == 1) draw_mesh();
}

/* the panel is rebuilt when the theme changes: the old timer has to go */
void other_ui_destroy(void)
{
    if(g_ui.timer) {
        lv_timer_delete(g_ui.timer);
        g_ui.timer = NULL;
    }
    /* an animation that outlives its object is a crash waiting to happen
       (the text cursor blinks) */
    lv_kill_tree(g_ui.root);
    g_ui.root = NULL;
    g_ui.content = NULL;
    g_ui.strip_title = NULL;
}

/* which page is open - the rebuild puts it back where it was */
void other_ui_get_state(int * page, int * sub)
{
    *page = g_ui.page;
    *sub = g_ui.sub;
}

void other_ui_set_state(int page, int sub)
{
    select_page(page);
    if(sub > 0) select_sub(sub);
}

/* ---------- the text fields on the User page ---------- */

/* the name other devices see: saved, and applied to the chat at once */
static void save_name(lv_obj_t * ta)
{
    if(!ta) return;
    const char * txt = lv_textarea_get_text(ta);
    if(!txt || !*txt) {          /* empty: keep the name we have */
        lv_textarea_set_text(ta, "");
        return;
    }
    prefs_set_str("name", txt);
    chat_set_identity(txt);
    lv_textarea_set_placeholder_text(ta, txt);
    lv_textarea_set_text(ta, "");
    flash_saved(g_ui.name_btn, g_ui.name_btn_lbl);
}

static void save_room(lv_obj_t * ta)
{
    if(!ta) return;
    const char * txt = lv_textarea_get_text(ta);
    if(!txt || !*txt) {          /* empty: keep the room we have */
        lv_textarea_set_text(ta, "");
        return;
    }
    prefs_set_str("room", txt);
    chat_set_room(txt);
    lv_textarea_set_placeholder_text(ta, txt);
    lv_textarea_set_text(ta, "");
    flash_saved(g_ui.room_btn, g_ui.room_btn_lbl);
}

void other_ui_on_name(lv_event_t * e)
{
    LV_UNUSED(e);
    lv_obj_t * ta = lv_event_get_current_target(e);
    save_name(ta);
}

void other_ui_on_room(lv_event_t * e)
{
    LV_UNUSED(e);
    lv_obj_t * ta = lv_event_get_current_target(e);
    save_room(ta);
}

static void on_save_name(lv_event_t * e)
{
    LV_UNUSED(e);
    save_name(g_ui.name_ta);
}

static void on_save_room(lv_event_t * e)
{
    LV_UNUSED(e);
    save_room(g_ui.room_ta);
}

/* the channel the local mesh talks on */
static void save_channel(lv_obj_t * ta)
{
    if(!ta) return;
    const char * txt = lv_textarea_get_text(ta);
    if(!txt || !*txt) {          /* empty: keep the channel we have */
        lv_textarea_set_text(ta, "");
        return;
    }
    prefs_set_str("mesh_chan", txt);
    chat_set_mesh_channel(txt);
    lv_textarea_set_placeholder_text(ta, txt);
    lv_textarea_set_text(ta, "");
    flash_saved(g_ui.chan_btn, g_ui.chan_btn_lbl);
}

void other_ui_on_channel(lv_event_t * e)
{
    LV_UNUSED(e);
    lv_obj_t * ta = lv_event_get_current_target(e);
    save_channel(ta);
}

static void on_save_channel(lv_event_t * e)
{
    LV_UNUSED(e);
    save_channel(g_ui.chan_ta);
}

/* the relay URL (spec-nstr-relay §7): the pref is plain text - a
   syntactically bad URL still saves and the carrier's parser is what says
   "bad url", nothing in this row pretends to validate more than it does.
   Clearing the field clears the pref (honest "no relay" when no env var is
   set); PDA_NSTR_RELAY still wins when it is, so clearing never overrides
   an operator's explicit URL. */
static void save_relay(lv_obj_t * ta)
{
    if(!ta) return;
    const char * txt = lv_textarea_get_text(ta);
    prefs_set_str("nstr_relay", (txt && *txt) ? txt : "");
    if(!txt || !*txt) lv_textarea_set_text(ta, "");
    flash_saved(g_ui.relay_btn, g_ui.relay_btn_lbl);
}

void other_ui_on_relay(lv_event_t * e)
{
    LV_UNUSED(e);
    lv_obj_t * ta = lv_event_get_current_target(e);
    save_relay(ta);
}

static void on_save_relay(lv_event_t * e)
{
    LV_UNUSED(e);
    save_relay(g_ui.relay_ta);
}

/* ---------- the identity actions on the User page ---------- */

/* [KEY]: the block appearing or vanishing *is* the feedback - it saves
   nothing, so it gets no flash */
static void on_key_toggle(lv_event_t * e)
{
    LV_UNUSED(e);
    s_show_key = !s_show_key;
    build_content();
}

/* [NEW]: first press arms ("NEW" -> "SURE?"), the second regenerates.
   On success the page rebuilds first - the nstr row showing the new key is
   the primary feedback - and only then is the flash armed on the rebuilt
   page's fresh label. On failure nothing changed: no rebuild, just FAILED
   in the failure colour (identity.c wrote the SYSTEM line saying why). */
static void on_new_click(lv_event_t * e)
{
    LV_UNUSED(e);
    lv_obj_t * lbl = g_ui.new_btn_lbl;
    if(!s_new_armed) {
        s_new_armed = true;
        if(lbl) lv_label_set_text(lbl, tr(TR_KEY_SURE));
        return;
    }
    s_new_armed = false;
    if(identity_regenerate()) {
        s_show_key = false;      /* whatever was revealed belongs to the
                                    old key now */
        build_content();
        flash_btn(g_ui.new_btn_lbl, tr(TR_SAVED), tr(TR_KEY_NEW),
                  PDA_CARRIER_UP);
    }
    else {
        flash_btn(lbl, tr(TR_FAILED), tr(TR_KEY_NEW), PDA_CARRIER_DOWN);
    }
}

static void do_paste(lv_obj_t * ta)
{
    if(!ta) return;
    const char * txt = lv_textarea_get_text(ta);
    if(!txt || !*txt) {          /* empty: nothing to import */
        lv_textarea_set_text(ta, "");
        return;
    }
    if(identity_import_nsec(txt)) {
        lv_textarea_set_text(ta, "");
        s_show_key = false;
        build_content();         /* the nstr row shows the imported key */
        flash_btn(g_ui.paste_btn_lbl, tr(TR_SAVED), tr(TR_KEY_PASTE),
                  PDA_CARRIER_UP);
    }
    else {
        /* invalid input = an honest error: no rebuild, nothing saved,
           FAILED on the button (identity.c said why on the log) */
        lv_textarea_set_text(ta, "");
        flash_btn(g_ui.paste_btn_lbl, tr(TR_FAILED), tr(TR_KEY_PASTE),
                  PDA_CARRIER_DOWN);
    }
}

static void other_ui_on_paste(lv_event_t * e)
{
    LV_UNUSED(e);
    do_paste(lv_event_get_current_target(e));
}

static void on_paste_click(lv_event_t * e)
{
    LV_UNUSED(e);
    do_paste(g_ui.paste_ta);
}

/* The shell calls this when the Other tab is left (set_tab only hides the
   panel, so the clear has to happen where it is hidden): the rule is that
   the nsec is on screen only immediately after an explicit press, on the
   visible User page, until anything else happens. */
void other_ui_hide_secrets(void)
{
    if(!s_show_key) return;
    s_show_key = false;
    if(g_ui.content) build_content();
}

/* ---------- build ---------- */

lv_obj_t * other_ui_create(lv_obj_t * parent, lv_group_t * group)
{
    memset(&g_ui, 0, sizeof(g_ui));
    g_ui.group = group;
    s_show_key = false;          /* a fresh panel: nothing is revealed */
    s_new_armed = false;

    lv_obj_t * root = lv_obj_create(parent);
    lv_obj_set_pos(root, 0, 0);
    lv_obj_set_size(root, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(root, lv_color_hex(PDA_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(root, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(root, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(root, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(root, LV_DIR_NONE);
    lv_obj_set_scrollable(root, false);
    g_ui.root = root;

    /* --- the gritty surface everything sits on --- */
    grit_backdrop(root, BACK_W, BACK_H);

    /* --- the strip under the tab bar: a title and the two indicators --- */
    grit_square(root, 22, 9, 8, PDA_MSG_ACCENT);
    grit_square(root, 770, 9, 8, PDA_MSG_ACCENT);

    g_ui.strip_title = label(root, 0, 0, 300, tr(g_page_ids[0]), &pda_font_mono_10,
                             PDA_MSG_TEXT);
    lv_obj_set_style_text_align(g_ui.strip_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(g_ui.strip_title, LV_ALIGN_TOP_MID, 0, 8);

    grit_rule(root, 6, STRIP_H + 2, 788, false, PDA_RULE);

    /* --- the frame around the panel --- */
    lv_obj_t * frame = grit_frame(root, FRAME_X, FRAME_Y, FRAME_W, FRAME_H, PDA_FRAME);
    if(frame) grit_corners(frame, FRAME_W, FRAME_H, PDA_FRAME);

    /* --- the left column --- */
    for(int i = 0; i < PAGE_COUNT; i++) {
        lv_obj_t * box = lv_button_create(root);
        lv_obj_set_size(box, NAV_W, NAV_H);
        lv_obj_set_pos(box, COL_X, PAGE_Y + i * NAV_PITCH);
        lv_obj_set_style_pad_all(box, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(box, 0, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(box, 0, LV_PART_MAIN);
        lv_obj_set_style_outline_width(box, 0, LV_PART_MAIN);
        lv_no_transition(box);
        g_ui.nav_box[i] = box;

        lv_obj_t * l = label(box, 0, 0, 0, tr(g_page_ids[i]), &pda_font_mono_14,
                             PDA_TAB_TEXT);
        lv_obj_set_width(l, NAV_W - 16);
        lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_obj_align(l, LV_ALIGN_LEFT_MID, 8, 0);   /* centred in the height */
        g_ui.nav_lbl[i] = l;

        lv_obj_set_clickable(box, true);
        lv_obj_add_event_cb(box, on_nav_click, LV_EVENT_CLICKED, (void *)(lv_intptr_t)i);
        if(group) lv_group_add_obj(group, box);
    }

    /* --- the sub tabs, down the second column --- */
    for(int i = 0; i < 8; i++) {
        lv_obj_t * b = lv_button_create(root);
        lv_obj_set_size(b, SUB_W, SUB_H);
        lv_obj_set_pos(b, SUB_X, PAGE_Y + i * SUB_PITCH);
        lv_obj_set_style_pad_all(b, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(b, 0, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(b, 0, LV_PART_MAIN);
        lv_obj_set_style_outline_width(b, 0, LV_PART_MAIN);
        lv_no_transition(b);
        g_ui.sub_btn[i] = b;

        lv_obj_t * l = label(b, 0, 0, 0, "", &pda_font_mono_10, PDA_TAB_TEXT);
        lv_obj_set_width(l, SUB_W - 8);
        lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_obj_align(l, LV_ALIGN_LEFT_MID, 6, 0);   /* centred in the height */
        g_ui.sub_lbl[i] = l;

        lv_obj_set_clickable(b, true);
        lv_obj_add_event_cb(b, on_sub_click, LV_EVENT_CLICKED, (void *)(lv_intptr_t)i);
        if(group) lv_group_add_obj(group, b);
    }

    /* --- the rules between the columns --- */
    grit_rule(root, RULE1_X, PAGE_Y, PAGE_H, true, PDA_FRAME_INNER);
    grit_rule(root, RULE2_X, PAGE_Y, PAGE_H, true, PDA_FRAME_INNER);

    /* --- the page --- */
    lv_obj_t * content = lv_obj_create(root);
    lv_obj_set_pos(content, PAGE_X, PAGE_Y);
    lv_obj_set_size(content, PAGE_W, PAGE_H);
    lv_obj_set_style_bg_color(content, lv_color_hex(PDA_PAGE_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(content, LV_OPA_90, LV_PART_MAIN);
    lv_obj_set_style_border_width(content, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(content, lv_color_hex(PDA_PAGE_EDGE), LV_PART_MAIN);
    lv_obj_set_style_pad_all(content, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_row(content, 2, LV_PART_MAIN);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_style_width(content, 4, LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_color(content, lv_color_hex(PDA_SCROLL), LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_opa(content, LV_OPA_COVER, LV_PART_SCROLLBAR);
    lv_obj_set_style_radius(content, 0, LV_PART_SCROLLBAR);
    lv_obj_set_style_pad_all(content, 0, LV_PART_SCROLLBAR);
    lv_obj_set_scroll_dir(content, LV_DIR_VER);
    lv_obj_set_scrollable(content, true);
    g_ui.content = content;

    /* test hooks: open a given page (PDA_OTHER_PAGE / PDA_OTHER_SUB). They
       only count at startup: after that the panel keeps the page it was on. */
    static bool first = true;
    int page = PAGE_SETTINGS;
    int sub = 0;
    if(first) {
        first = false;
        const char * p_env = getenv("PDA_OTHER_PAGE");
        const char * s_env = getenv("PDA_OTHER_SUB");
        if(p_env && *p_env) page = atoi(p_env);
        if(s_env && *s_env) sub = atoi(s_env);
    }
    select_page(page);
    if(sub > 0) select_sub(sub);
    g_ui.timer = lv_timer_create(other_ui_tick, 500, NULL);
    return root;
}
