#include "ui/other_ui.h"
#include "ui/palette.h"
#include "ui/prefs.h"
#include "ui/lvutil.h"
#include "assets/fonts/pda_fonts.h"
#include "chat/chat.h"
#include "gps.h"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netpacket/packet.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* The Other tab.
 *
 *   +--------------+ +------------------------------------------+
 *   | Settings     | | General | Advanced | Language | Themes |
 *   | Connections  | +------------------------------------------+
 *   | User         | |                                          |
 *   | Extras       | |                                          |
 *   +--------------+ +------------------------------------------+
 *
 * Settings and Extras are placeholders for now; Connections and User work.
 */

#define PAGE_COUNT   4
#define NAV_W        186
#define SUB_H        19
#define SUB_X0       (NAV_W + 10)
#define SUB_W        92
#define SUB_GAP      3
#define ROW_H        18
#define VAL_X        158

enum { PAGE_SETTINGS = 0, PAGE_CONNECTIONS = 1, PAGE_USER = 2, PAGE_EXTRAS = 3 };

static const char * const g_page_names[PAGE_COUNT] = {
    "Settings", "Connections", "User", "Extras"
};

static const char * const g_sub_names[PAGE_COUNT][8] = {
    { "General", "Advanced", "Language", "Themes" },
    { "Info", "Bitchat", "F.I.P.S", "WiFi", "GPS", "System" },
    { NULL },
    { NULL },
};

static void other_ui_refresh(void);
static void no_theme_transition(lv_obj_t * o);
static void other_ui_on_name(lv_event_t * e);
static void other_ui_on_room(lv_event_t * e);
static void on_save_name(lv_event_t * e);
static void on_save_room(lv_event_t * e);
static void on_toggle_click(lv_event_t * e);

typedef struct {
    lv_obj_t * root;
    lv_group_t * group;
    lv_obj_t * nav_box[PAGE_COUNT];
    lv_obj_t * nav_lbl[PAGE_COUNT];
    lv_obj_t * sub_btn[8];
    lv_obj_t * sub_lbl[8];
    int n_subs;
    lv_obj_t * content;
    int page;
    int sub;

    /* the live labels of the page that is on screen */
    lv_obj_t * live[16];
    int n_live;
    char live_key[16][24];   /* what each live label shows */

    /* the toggles on the User page */
    lv_obj_t * tog[4];
    char tog_key[4][24];
    int n_tog;

    lv_obj_t * name_ta;
    lv_obj_t * room_ta;
    lv_obj_t * name_btn;
    lv_obj_t * room_btn;
    lv_obj_t * name_btn_lbl;
    lv_obj_t * room_btn_lbl;
} other_ui_t;

static other_ui_t g_ui;
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

/* a section heading */
static void section(lv_obj_t * parent, const char * title)
{
    lv_obj_t * l = label(parent, 0, 0, 0, title, &pda_font_mono_10, PDA_MSG_ACCENT);
    lv_obj_set_height(l, 14);
}

/* one "setting" row: a name on the left, the value on the right */
static lv_obj_t * row(lv_obj_t * parent, const char * name)
{
    lv_obj_t * r = plain(parent, lv_pct(100), ROW_H);
    lv_obj_set_flex_grow(r, 0);
    label(r, 0, 3, 150, name, &pda_font_mono_10, PDA_MSG_TIME);
    return r;
}

static lv_obj_t * value(lv_obj_t * r, const char * text, uint32_t color)
{
    lv_obj_t * v = label(r, VAL_X, 1, 380, text, &pda_font_mono_10, color);
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
        out[n].up = tr->up;
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
        lv_obj_t * r = live_row(g_ui.content, st[i].name, key,
                                st[i].up ? "CONNECTED" : "OFFLINE",
                                st[i].up ? PDA_CARRIER_UP : PDA_CARRIER_DOWN);
        (void)r;
    }

    section(g_ui.content, "HARDWARE");
    live_row(g_ui.content, "WiFi", "wifi", wifi_up() ? "CONNECTED" : "OFFLINE",
             wifi_up() ? PDA_CARRIER_UP : PDA_CARRIER_DOWN);
    const gps_fix_t * g = gps_current();
    live_row(g_ui.content, "GPS", "gps",
             g->valid ? (g->simulated ? "SIMULATED" : "FIX") : "NO FIX",
             g->valid ? PDA_CARRIER_UP : PDA_CARRIER_DOWN);
    live_row(g_ui.content, "System", "system", "OK", PDA_CARRIER_UP);
}

static void page_bitchat(void)
{
    const char * sock = getenv("PDA_BITCHAT_SOCK");
    const char * host = getenv("PDA_BITCHAT_HOST");
    char buf[128];

    section(g_ui.content, "BITCHAT - LOCAL MESH");
    snprintf(buf, sizeof(buf), "%s", getenv("PDA_BITCHAT_PORT") ? getenv("PDA_BITCHAT_PORT")
                                                                 : "2425");
    live_row(g_ui.content, "Mesh port", "bit_port", buf, PDA_MSG_TEXT);
    live_row(g_ui.content, "Bridge", "bit_bridge",
             (sock && *sock) ? sock : ((host && *host) ? host : "not connected"),
             (sock || host) ? PDA_CARRIER_UP : PDA_CARRIER_DOWN);
    live_row(g_ui.content, "Node name", "bit_name", chat_identity(), PDA_MSG_TEXT);
    live_row(g_ui.content, "Channel", "bit_chan", "#pda-zone", PDA_MSG_TEXT);

    section(g_ui.content, "LINK");
    live_row(g_ui.content, "State", "bit_state", "offline", PDA_CARRIER_DOWN);
    live_row(g_ui.content, "Peers heard", "bit_peers", "-", PDA_MSG_TEXT);
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
    snprintf(buf, sizeof(buf), "%.5f", g->lat);
    live_row(g_ui.content, "Latitude", "gps_lat", buf, PDA_MSG_TEXT);
    snprintf(buf, sizeof(buf), "%.5f", g->lon);
    live_row(g_ui.content, "Longitude", "gps_lon", buf, PDA_MSG_TEXT);
    snprintf(buf, sizeof(buf), "%d", g->sats);
    live_row(g_ui.content, "Satellites", "gps_sats", buf, PDA_MSG_TEXT);
    snprintf(buf, sizeof(buf), "%.1f m/s", g->speed);
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
    live_row(g_ui.content, "Settings file", "sys_prefs", prefs_path(), PDA_MSG_TEXT);
}

static void page_settings(int sub)
{
    static const char * const planned[4][5] = {
        { "Device name", "Time format", "Units", "Startup screen", NULL },
        { "Performance", "Logging", "Reset log", NULL, NULL },
        { "Language", "Keyboard layout", "Region", NULL, NULL },
        { "Colour scheme", "Accent colour", "Font size", NULL, NULL },
    };
    const char * title = g_sub_names[PAGE_SETTINGS][sub];

    section(g_ui.content, title);
    for(int i = 0; i < 5 && planned[sub][i]; i++)
        live_row(g_ui.content, planned[sub][i], "planned", "not wired up yet",
                 PDA_MSG_SYS);
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
    lv_label_set_text(l, "SAVE");
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
        lv_label_set_text(l, "SAVE");
        lv_obj_set_style_text_color(l, lv_color_hex(PDA_MSG_ACCENT), LV_PART_MAIN);
    }
    s_flash_timer = NULL;      /* the timer deletes itself: say so */
    lv_timer_delete(timer);
}

static void flash_saved(lv_obj_t * btn, lv_obj_t * lbl)
{
    if(!btn || !lbl) return;
    lv_label_set_text(lbl, "SAVED");
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

/* a row with a switch on the right */
static void toggle_row(lv_obj_t * parent, const char * name, const char * key)
{
    lv_obj_t * r = row(parent, name);
    bool on = prefs_bool(key, true);
    lv_obj_t * b = toggle(r, on);
    lv_obj_set_pos(b, VAL_X, 2);
    lv_obj_add_event_cb(b, on_toggle_click, LV_EVENT_CLICKED,
                        (void *)(lv_intptr_t)(g_ui.n_tog));
    if(g_ui.n_tog < 4) {
        g_ui.tog[g_ui.n_tog] = b;
        snprintf(g_ui.tog_key[g_ui.n_tog], sizeof(g_ui.tog_key[0]), "%s", key);
        g_ui.n_tog++;
    }
}

static void page_user(void)
{
    section(g_ui.content, "IDENTITY");

    lv_obj_t * r = row(g_ui.content, "Name");
    lv_obj_t * ta = lv_textarea_create(r);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_placeholder_text(ta, chat_identity());
    lv_obj_set_pos(ta, VAL_X, 0);
    lv_obj_set_size(ta, 220, 17);
    lv_obj_set_style_bg_color(ta, lv_color_hex(PDA_CHAT_INPUT), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ta, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(ta, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(ta, lv_color_hex(PDA_CHAT_EDGE), LV_PART_MAIN);
    lv_obj_set_style_text_font(ta, &pda_font_mono_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(ta, lv_color_hex(PDA_MSG_TEXT), LV_PART_MAIN);
    lv_obj_set_style_text_color(ta, lv_color_hex(PDA_MSG_SYS), LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_pad_all(ta, 2, LV_PART_MAIN);
    lv_obj_add_event_cb(ta, other_ui_on_name, LV_EVENT_READY, NULL);
    if(g_ui.group) lv_group_add_obj(g_ui.group, ta);
    g_ui.name_ta = ta;
    g_ui.name_btn = save_button(r, on_save_name);
    g_ui.name_btn_lbl = lv_obj_get_child(g_ui.name_btn, 0);

    lv_obj_t * r2 = row(g_ui.content, "Room");
    lv_obj_t * ta2 = lv_textarea_create(r2);
    lv_textarea_set_one_line(ta2, true);
    lv_textarea_set_placeholder_text(ta2, chat_room());
    lv_obj_set_pos(ta2, VAL_X, 0);
    lv_obj_set_size(ta2, 220, 17);
    lv_obj_set_style_bg_color(ta2, lv_color_hex(PDA_CHAT_INPUT), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ta2, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(ta2, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(ta2, lv_color_hex(PDA_CHAT_EDGE), LV_PART_MAIN);
    lv_obj_set_style_text_font(ta2, &pda_font_mono_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(ta2, lv_color_hex(PDA_MSG_TEXT), LV_PART_MAIN);
    lv_obj_set_style_text_color(ta2, lv_color_hex(PDA_MSG_SYS), LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_pad_all(ta2, 2, LV_PART_MAIN);
    lv_obj_add_event_cb(ta2, other_ui_on_room, LV_EVENT_READY, NULL);
    if(g_ui.group) lv_group_add_obj(g_ui.group, ta2);
    g_ui.room_ta = ta2;
    g_ui.room_btn = save_button(r2, on_save_room);
    g_ui.room_btn_lbl = lv_obj_get_child(g_ui.room_btn, 0);

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
    g_ui.name_ta = NULL;
    g_ui.room_ta = NULL;

    if(g_ui.page == PAGE_SETTINGS) page_settings(g_ui.sub);
    else if(g_ui.page == PAGE_CONNECTIONS) {
        switch(g_ui.sub) {
            case 0: page_info(); break;
            case 1: page_bitchat(); break;
            case 2: page_fips(); break;
            case 3: page_wifi(); break;
            case 4: page_gps(); break;
            default: page_system(); break;
        }
    }
    else if(g_ui.page == PAGE_USER) page_user();
    else page_extras();

    other_ui_refresh();
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
            if(idx < n)
                set_live(i, st[idx].up ? "CONNECTED" : "OFFLINE",
                         st[idx].up ? PDA_CARRIER_UP : PDA_CARRIER_DOWN);
        }
        else if(!strcmp(key, "bit_state")) {
            const chat_transport_t * tr = NULL;
            for(int k = 0; k < chat_transport_count(); k++) {
                const chat_transport_t * t = chat_transport_get(k);
                if(t && !strcmp(t->tag, "BIT")) tr = t;
            }
            set_live(i, (tr && tr->up) ? "ONLINE" : "OFFLINE",
                     (tr && tr->up) ? PDA_CARRIER_UP : PDA_CARRIER_DOWN);
        }
        else if(!strcmp(key, "fips_state")) {
            const chat_transport_t * tr = NULL;
            for(int k = 0; k < chat_transport_count(); k++) {
                const chat_transport_t * t = chat_transport_get(k);
                if(t && !strcmp(t->tag, "FIPS")) tr = t;
            }
            set_live(i, (tr && tr->up) ? "ONLINE" : "OFFLINE",
                     (tr && tr->up) ? PDA_CARRIER_UP : PDA_CARRIER_DOWN);
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
            snprintf(b, sizeof(b), "%.5f", gps_current()->lat);
            set_live(i, b, PDA_MSG_TEXT);
        }
        else if(!strcmp(key, "gps_lon")) {
            char b[32];
            snprintf(b, sizeof(b), "%.5f", gps_current()->lon);
            set_live(i, b, PDA_MSG_TEXT);
        }
        else if(!strcmp(key, "gps_sats")) {
            char b[32];
            snprintf(b, sizeof(b), "%d", gps_current()->sats);
            set_live(i, b, PDA_MSG_TEXT);
        }
        else if(!strcmp(key, "gps_speed")) {
            char b[32];
            snprintf(b, sizeof(b), "%.1f m/s", gps_current()->speed);
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
        uint32_t bg = on ? PDA_SUB_FILL : PDA_CHAT_BG;
        lv_state_t st;
        for(int k = 0; k < 2; k++) {
            st = k ? LV_STATE_FOCUSED : LV_PART_MAIN;
            lv_obj_set_style_bg_color(g_ui.nav_box[i], lv_color_hex(bg), st);
            lv_obj_set_style_bg_opa(g_ui.nav_box[i], LV_OPA_COVER, st);
            lv_obj_set_style_border_side(g_ui.nav_box[i],
                                          on ? LV_BORDER_SIDE_LEFT : LV_BORDER_SIDE_NONE,
                                          st);
            lv_obj_set_style_border_color(g_ui.nav_box[i], lv_color_hex(PDA_MSG_ACCENT),
                                          st);
            lv_obj_set_style_border_width(g_ui.nav_box[i], on ? 3 : 0, st);
            lv_obj_set_style_outline_width(g_ui.nav_box[i], 0, st);
            lv_obj_set_style_shadow_width(g_ui.nav_box[i], 0, st);
            lv_obj_set_style_bg_color(g_ui.nav_box[i], lv_color_hex(bg), LV_STATE_PRESSED);
        }
        lv_obj_set_style_text_color(g_ui.nav_lbl[i],
                                    lv_color_hex(on ? PDA_MSG_TEXT : PDA_TAB_TEXT),
                                    LV_PART_MAIN);
    }
}

static void restyle_subs(void)
{
    for(int i = 0; i < g_ui.n_subs; i++) {
        bool on = (i == g_ui.sub);
        uint32_t bg = on ? PDA_SUB_FILL : PDA_CHAT_BG;
        lv_state_t st;
        for(int k = 0; k < 2; k++) {
            st = k ? LV_STATE_FOCUSED : LV_PART_MAIN;
            lv_obj_set_style_bg_color(g_ui.sub_btn[i], lv_color_hex(bg), st);
            lv_obj_set_style_bg_opa(g_ui.sub_btn[i], LV_OPA_COVER, st);
            lv_obj_set_style_border_side(g_ui.sub_btn[i], LV_BORDER_SIDE_TOP, st);
            lv_obj_set_style_border_color(g_ui.sub_btn[i],
                                          lv_color_hex(on ? PDA_MSG_ACCENT : PDA_CHAT_EDGE),
                                          st);
            lv_obj_set_style_border_width(g_ui.sub_btn[i], on ? 2 : 1, st);
            lv_obj_set_style_outline_width(g_ui.sub_btn[i], 0, st);
            lv_obj_set_style_shadow_width(g_ui.sub_btn[i], 0, st);
            lv_obj_set_style_bg_color(g_ui.sub_btn[i], lv_color_hex(bg), LV_STATE_PRESSED);
        }
        lv_obj_set_style_text_color(g_ui.sub_lbl[i],
                                    lv_color_hex(on ? PDA_MSG_TEXT : PDA_TAB_TEXT),
                                    LV_PART_MAIN);
    }
}

static void select_page(int page)
{
    if(page < 0 || page >= PAGE_COUNT) return;
    g_ui.page = page;
    g_ui.sub = 0;
    int n = 0;
    while(n < 8 && g_sub_names[page][n]) n++;
    g_ui.n_subs = n;
    for(int i = 0; i < 8; i++) {
        if(!g_ui.sub_btn[i]) continue;
        bool show = (i < n);
        lv_obj_set_hidden(g_ui.sub_btn[i], !show);
        if(show) lv_label_set_text(g_ui.sub_lbl[i], g_sub_names[page][i]);
    }
    restyle_nav();
    restyle_subs();
    build_content();
}

static void select_sub(int sub)
{
    if(sub < 0 || sub >= g_ui.n_subs) return;
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

static void other_ui_tick(lv_timer_t * timer)
{
    LV_UNUSED(timer);
    other_ui_refresh();
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

/* ---------- build ---------- */

lv_obj_t * other_ui_create(lv_obj_t * parent, lv_group_t * group)
{
    memset(&g_ui, 0, sizeof(g_ui));
    g_ui.group = group;

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

    /* --- the left column --- */
    for(int i = 0; i < PAGE_COUNT; i++) {
        lv_obj_t * box = lv_button_create(root);
        lv_obj_set_size(box, NAV_W - 12, 26);
        lv_obj_set_pos(box, 6, 8 + i * 30);
        lv_obj_set_style_pad_all(box, 8, LV_PART_MAIN);
        lv_obj_set_style_radius(box, 0, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(box, 0, LV_PART_MAIN);
        lv_obj_set_style_outline_width(box, 0, LV_PART_MAIN);
        lv_no_transition(box);
        g_ui.nav_box[i] = box;

        lv_obj_t * l = label(box, 0, 0, 0, g_page_names[i], &pda_font_mono_14,
                             PDA_TAB_TEXT);
        lv_obj_set_pos(l, 4, 4);
        g_ui.nav_lbl[i] = l;

        lv_obj_set_clickable(box, true);
        lv_obj_add_event_cb(box, on_nav_click, LV_EVENT_CLICKED, (void *)(lv_intptr_t)i);
        if(group) lv_group_add_obj(group, box);
    }

    /* --- the sub tabs --- */
    for(int i = 0; i < 8; i++) {
        lv_obj_t * b = lv_button_create(root);
        lv_obj_set_size(b, SUB_W, SUB_H);
        lv_obj_set_pos(b, SUB_X0 + i * (SUB_W + SUB_GAP), 4);
        lv_obj_set_style_pad_all(b, 2, LV_PART_MAIN);
        lv_obj_set_style_radius(b, 0, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(b, 0, LV_PART_MAIN);
        lv_obj_set_style_outline_width(b, 0, LV_PART_MAIN);
        lv_no_transition(b);
        g_ui.sub_btn[i] = b;

        lv_obj_t * l = label(b, 0, 0, 0, "", &pda_font_mono_10, PDA_TAB_TEXT);
        lv_obj_set_pos(l, 2, 3);
        lv_obj_set_width(l, SUB_W - 4);
        lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        g_ui.sub_lbl[i] = l;

        lv_obj_set_clickable(b, true);
        lv_obj_add_event_cb(b, on_sub_click, LV_EVENT_CLICKED, (void *)(lv_intptr_t)i);
        if(group) lv_group_add_obj(group, b);
    }

    /* --- the page --- */
    lv_obj_t * content = lv_obj_create(root);
    lv_obj_set_pos(content, SUB_X0 - 6, 4 + SUB_H + 6);
    lv_obj_set_size(content, 800 - SUB_X0 + 6, 400);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(content, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(content, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_row(content, 2, LV_PART_MAIN);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(content, LV_DIR_VER);
    lv_obj_set_scrollable(content, true);
    g_ui.content = content;

    /* test hooks: open a given page (PDA_OTHER_PAGE / PDA_OTHER_SUB) */
    int page = PAGE_CONNECTIONS;
    int sub = 0;
    const char * p_env = getenv("PDA_OTHER_PAGE");
    const char * s_env = getenv("PDA_OTHER_SUB");
    if(p_env && *p_env) page = atoi(p_env);
    if(s_env && *s_env) sub = atoi(s_env);
    select_page(page);
    if(sub > 0) select_sub(sub);
    lv_timer_create(other_ui_tick, 500, NULL);
    return root;
}
