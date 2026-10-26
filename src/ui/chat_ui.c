#include "ui/chat_ui.h"
#include "ui/palette.h"
#include "ui/grit.h"
#include "ui/i18n.h"
#include "ui/format.h"
#include "assets/fonts/pda_fonts.h"
#include "chat/chat.h"
#include "identity/contacts.h"
#include "ui/prefs.h"
#include "ui/lvutil.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* The Messages tab.
 *
 * One room, one log. Everything that arrives - over the local mesh, over the
 * FIPS mesh, over the internet - shows up in the same list, tagged with the
 * carrier it came in on, and everything typed here goes out on every carrier
 * that is up.
 *
 * The guide look: a gritty surface, a framed panel with the log in it and
 * the compose row at the bottom of it.
 *
 *   +-------------------------------------------------------------+
 *   | SECTOR   mesh 2425 / relay.example.com  SEND ON (BIT) (F)   |
 *   |                                            BIT  FIPS  NSTR  |
 *   +-------------------------------------------------------------+
 *   | 12:05  ME       not yet, rain again                        |
 *   | 12:04  ROOK     anyone got the drop coords?                 |
 *   |-------------------------------------------------------------|
 *   | [ type a message...                                ] [ SEND ] |
 *   +-------------------------------------------------------------+
 *
 * The newest message is at the top of the log, and the buttons in the
 * strip say which carriers a message goes out on - one, two or all of them.
 */

#define LIST_W        800
#define STRIP_H       26
#define PICK_W        66      /* a carrier button */
#define PICK_H        18
#define PICK_GAP      4
#define PICK_RIGHT    636     /* where the buttons end (the badges are at 646) */
#define DISC_N        12      /* the little disc in the button */
#define COMPOSE_H     40
#define ROW_MAX       120
#define TEXT_X        152
#define TEXT_W        596
#define NAME_X        44
#define NAME_W        62
#define TAG_X         108
#define TAG_W         42
#define FRAME_X       6
#define FRAME_Y       32
#define FRAME_W       788
#define FRAME_H       407
#define BACK_W        800
#define BACK_H        452

typedef struct {
    lv_obj_t * root;
    lv_obj_t * list;
    lv_obj_t * room_lbl;
    lv_obj_t * status_lbl;
    lv_obj_t * badge[CHAT_TRANSPORT_MAX];
    lv_obj_t * pick_btn[CHAT_TRANSPORT_MAX];
    lv_obj_t * pick_lbl[CHAT_TRANSPORT_MAX];
    lv_obj_t * pick_disc[CHAT_TRANSPORT_MAX];
    uint32_t pick_buf[CHAT_TRANSPORT_MAX][DISC_N * DISC_N];
    lv_obj_t * input;
    lv_obj_t * send_btn;
    lv_timer_t * timer;
    int rendered;         /* messages already drawn */
    int rows;              /* rows currently in the list */
    int y;                 /* the height of all the rows together */
    int row_h[ROW_MAX];    /* height of each row, for the scroll-back */
    unsigned c_ver;        /* contacts_version() when the list was drawn:
                              Trust/REMOVE in the Contacts tab has to
                              refresh the badge on rows already on screen */
} chat_ui_t;

static chat_ui_t g_ui;

static void chat_tick(lv_timer_t * timer);
static void refresh(void);
static void rebuild(void);
static void scroll_to_top(void);

/* PDA_DEBUG_FIELD: the absolute box of one compose-row object, the same
   hook the Other tab's rows print. Measured, not assumed - the frame these
   sit in adds its own border, so source constants are one pixel-cluster
   away from where a click has to land. Printed on the first tick, not at
   create(): the shell has only then finished putting the tab panel under
   the tab bar, and a box read earlier is that many pixels low. */
static void debug_field(const char * name, lv_obj_t * o)
{
    if(!o || !getenv("PDA_DEBUG_FIELD")) return;
    lv_obj_update_layout(o);
    lv_area_t a;
    lv_obj_get_coords(o, &a);
    fprintf(stderr, "[field] chat: %s x=%d y=%d w=%d h=%d\n",
            name, a.x1, a.y1, lv_obj_get_width(o), lv_obj_get_height(o));
}

static void debug_fields_once(void)
{
    static bool done = false;
    if(done || !getenv("PDA_DEBUG_FIELD")) return;
    done = true;
    debug_field("input", g_ui.input);
    debug_field("send", g_ui.send_btn);
}

/* the panel is rebuilt when the theme changes: the old timer has to go */
void chat_ui_destroy(void)
{
    if(g_ui.timer) {
        lv_timer_delete(g_ui.timer);
        g_ui.timer = NULL;
    }
    /* an animation that outlives its object is a crash waiting to happen
       (the text cursor blinks) */
    lv_kill_tree(g_ui.root);
    g_ui.root = NULL;
    g_ui.list = NULL;
    g_ui.input = NULL;
}

/* how often the log is looked at */
void chat_ui_set_poll(int ms)
{
    if(ms < 50) ms = 50;
    if(g_ui.timer) lv_timer_set_period(g_ui.timer, ms);
}

/* empty the log */
void chat_ui_clear(void)
{
    chat_clear();
    if(!g_ui.list) return;
    lv_kill_tree(g_ui.list);
    lv_obj_clean(g_ui.list);
    g_ui.rows = 0;
    g_ui.y = 0;
    g_ui.rendered = 0;
    scroll_to_top();
    refresh();
}

static void scroll_to_top(void)
{
    lv_obj_scroll_to_y(g_ui.list, 0, LV_ANIM_OFF);
}

/* is the log showing its newest line? */
static bool at_top(void)
{
    int y = lv_obj_get_scroll_y(g_ui.list);
    return y <= 4;
}

static void add_row(const chat_msg_t * m)
{
    char tbuf[16];
    time_t t = m->ts;
    fmt_clock(t, tbuf, sizeof(tbuf));

    const char * text = m->text;
    uint32_t name_col = PDA_MSG_SYS;
    const lv_font_t * nfont = &pda_font_mono_10;

    if(m->kind == CHAT_KIND_SYS) {
        name_col = PDA_MSG_SYS;
    }
    else if(m->outbound) {
        name_col = PDA_MSG_ME;
    }
    else {
        name_col = PDA_MSG_THEM;
    }

    /* the columns move depending on what the user wants to see */
    bool show_time = prefs_bool("show_time", true);
    bool show_carrier = prefs_bool("show_carrier", true);
    int time_x = 4;
    int name_x = show_time ? 44 : 4;
    int tag_x = show_time ? 108 : 68;
    int text_x = show_time ? TEXT_X : TEXT_X - 44;

    if(g_ui.rows >= ROW_MAX) {
        /* the oldest row goes: it is the first one that was made, and with
           the newest at the top it is the one at the bottom */
        lv_obj_t * old = lv_obj_get_child(g_ui.list, 0);
        if(old) {
            lv_kill_tree(old);
            lv_obj_delete(old);
        }
        for(int i = 0; i < g_ui.rows - 1; i++) g_ui.row_h[i] = g_ui.row_h[i + 1];
        g_ui.y -= g_ui.row_h[g_ui.rows - 1];
        g_ui.rows--;
    }

    lv_obj_t * row = lv_obj_create(g_ui.list);
    lv_obj_set_pos(row, 0, 0);
    lv_obj_set_size(row, lv_pct(100), 20);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(row, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(row, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(row, LV_DIR_NONE);
    lv_obj_set_scrollable(row, false);
    lv_obj_set_clickable(row, false);

    if(show_time) {
        lv_obj_t * tl = lv_label_create(row);
        lv_label_set_text(tl, tbuf);
        lv_obj_set_pos(tl, time_x, 1);
        lv_obj_set_width(tl, 38);
        lv_obj_set_style_text_font(tl, &pda_font_mono_10, LV_PART_MAIN);
        lv_obj_set_style_text_color(tl, lv_color_hex(PDA_MSG_TIME), LV_PART_MAIN);
        lv_obj_set_style_pad_all(tl, 0, LV_PART_MAIN);
    }

    lv_obj_t * nl = lv_label_create(row);
    lv_label_set_text(nl, m->sender);
    lv_obj_set_pos(nl, name_x, 1);
    lv_obj_set_width(nl, NAME_W);
    lv_label_set_long_mode(nl, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(nl, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_obj_set_style_text_font(nl, nfont, LV_PART_MAIN);
    lv_obj_set_style_text_color(nl, lv_color_hex(name_col), LV_PART_MAIN);
    lv_obj_set_style_pad_all(nl, 0, LV_PART_MAIN);

    /* the verified mark: inbound, signature verified, AND the key is one
       the user pressed Trust on. Anything else (unsigned, bad sig,
       unknown key, our own rows) shows no mark, ever. It sits in the name
       column - 7px of the existing carrier-green, no new font glyph - so
       it clears the time at x=42 and the carrier tag at x=108 */
    if(!m->outbound && m->sig == CHAT_SIG_OK && m->pub[0] && contacts_has(m->pub)) {
        lv_obj_t * dot = lv_obj_create(row);
        lv_obj_set_pos(dot, name_x, 5);
        lv_obj_set_size(dot, 7, 7);
        lv_obj_set_style_bg_color(dot, lv_color_hex(PDA_CARRIER_UP), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(dot, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_pad_all(dot, 0, LV_PART_MAIN);
        lv_obj_set_clickable(dot, false);
        lv_obj_set_scrollbar_mode(dot, LV_SCROLLBAR_MODE_OFF);
        /* the name slides right so the mark never sits on the letters */
        lv_obj_set_pos(nl, name_x + 10, 1);
        lv_obj_set_width(nl, NAME_W - 10);
    }

    /* the carrier it came in on - or, for our own messages, the ones it
       went out on */
    if(show_carrier) {
        const char * tag = NULL;
        if(m->transport >= 0 && m->transport < chat_transport_count()) {
            const chat_transport_t * tr = chat_transport_get(m->transport);
            if(tr) tag = tr->tag;
        }
        else if(m->outbound) {
            static char tags[32];
            tags[0] = 0;
            for(int i = 0; i < chat_transport_count(); i++) {
                const chat_transport_t * tr = chat_transport_get(i);
                if(!tr || tr->state != CHAT_CARRIER_ONLINE) continue;
                /* only the carriers the user wants are used */
                if(!chat_carrier_enabled(i)) continue;
                if(tags[0]) strncat(tags, " ", sizeof(tags) - strlen(tags) - 1);
                strncat(tags, tr->tag, sizeof(tags) - strlen(tags) - 1);
            }
            tag = tags[0] ? tags : NULL;
        }
        if(tag) {
            lv_obj_t * cl = lv_label_create(row);
            lv_label_set_text(cl, tag);
            lv_obj_set_pos(cl, tag_x, 2);
            lv_obj_set_width(cl, TAG_W);
            lv_obj_set_style_text_font(cl, &pda_font_mono_10, LV_PART_MAIN);
            lv_obj_set_style_text_color(cl, lv_color_hex(PDA_CARRIER_UP), LV_PART_MAIN);
            lv_obj_set_style_pad_all(cl, 0, LV_PART_MAIN);
        }
    }

    /* the label lays the text out itself: let it, and take its height */
    lv_obj_t * xl = lv_label_create(row);
    lv_label_set_text(xl, text);
    lv_obj_set_pos(xl, text_x, 0);
    lv_obj_set_width(xl, TEXT_W + (show_time ? 0 : 44));
    lv_label_set_long_mode(xl, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_pad_all(xl, 0, LV_PART_MAIN);
    lv_obj_set_style_text_font(xl, &pda_font_mono_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(xl, lv_color_hex(PDA_MSG_TEXT), LV_PART_MAIN);
    lv_obj_update_layout(xl);
    int th = lv_obj_get_height(xl);
    if(th <= 0) th = 14;
    lv_obj_set_height(row, th + 6);

    /* the newest message is at the top: the rows that are already there
       move down by the height of the new one */
    int h = th + 6;
    for(int i = 0; i < g_ui.rows; i++) {
        lv_obj_t * c = lv_obj_get_child(g_ui.list, i);
        if(c) lv_obj_set_pos(c, 0, lv_obj_get_y(c) + h);
    }

    g_ui.row_h[g_ui.rows] = th + 6;
    g_ui.rows++;
    g_ui.y += th + 6;

    /* the hairline between the messages */
    grit_rule(row, 0, th + 5, lv_pct(100), false, PDA_RULE);
}
/* A full rebuild of the log: clean list, reset rows/y/rendered, re-add
   every message. Extracted from refresh()'s trim branch so Trust/REMOVE in
   the Contacts tab (a contacts_version() change) can take the same path
   and repaint the verified marks.

   Scroll on a rebuild: the layout is recreated with the same rows, so the
   previous raw offset is restored as-is (clamped to the new max), or 0 if
   the view was at the top. Deliberately NOT refresh()'s old_y + added
   rule, which only makes sense for incremental prepends that shift
   content down - copying it here would fling the view to the bottom. */
static void rebuild(void)
{
    int old_y = lv_obj_get_scroll_y(g_ui.list);
    bool stick = old_y <= 4;

    lv_kill_tree(g_ui.list);
    lv_obj_clean(g_ui.list);
    g_ui.rows = 0;
    g_ui.y = 0;
    g_ui.rendered = 0;

    bool show_system = prefs_bool("show_system", true);
    int count = chat_count();
    for(int i = 0; i < count; i++) {
        const chat_msg_t * m = chat_get(i);
        if(!m) continue;
        if(m->kind == CHAT_KIND_SYS && !show_system) continue;
        add_row(m);
    }
    g_ui.rendered = count;
    g_ui.c_ver = contacts_version();

    lv_obj_update_layout(g_ui.list);
    if(stick) {
        lv_obj_scroll_to_y(g_ui.list, 0, LV_ANIM_OFF);
    }
    else {
        int32_t max_y = lv_obj_get_scroll_y(g_ui.list) +
                        lv_obj_get_scroll_bottom(g_ui.list);
        int32_t y = old_y;
        if(y > max_y) y = max_y;
        if(y < 0) y = 0;
        lv_obj_scroll_to_y(g_ui.list, y, LV_ANIM_OFF);
    }
}

static void refresh(void)
{
    int count = chat_count();
    bool stick = at_top();
    int old_y = lv_obj_get_scroll_y(g_ui.list);
    if(count < g_ui.rendered) {     /* the log was trimmed: rebuild */
        rebuild();
        return;
    }
    bool show_system = prefs_bool("show_system", true);
    int added = 0;
    for(int i = g_ui.rendered; i < count; i++) {
        const chat_msg_t * m = chat_get(i);
        if(!m) continue;
        if(m->kind == CHAT_KIND_SYS && !show_system) continue;
        int before = g_ui.y;
        add_row(m);
        added += g_ui.y - before;
    }
    g_ui.rendered = count;
    if(added > 0) {
        /* the new rows are at the top: the view stays where it was, and
           the newest one is in sight when the log was at the top */
        if(stick) lv_obj_scroll_to_y(g_ui.list, 0, LV_ANIM_OFF);
        else lv_obj_scroll_to_y(g_ui.list, old_y + added, LV_ANIM_OFF);
    }

    /* the carrier strip */
    char status[128] = "";
    int n = chat_transport_count();
    for(int i = 0; i < n && i < CHAT_TRANSPORT_MAX; i++) {
        const chat_transport_t * tr = chat_transport_get(i);
        if(!tr) continue;
        uint32_t col = lv_carrier_color(tr->state);
        if(g_ui.badge[i]) {
            lv_obj_set_style_text_color(g_ui.badge[i], lv_color_hex(col), LV_PART_MAIN);
        }
        if(tr->state == CHAT_CARRIER_ONLINE && tr->status[0]) {
            if(status[0]) strncat(status, "  /  ", sizeof(status) - strlen(status) - 1);
            strncat(status, tr->status, sizeof(status) - strlen(status) - 1);
        }
    }
    if(g_ui.status_lbl) lv_label_set_text(g_ui.status_lbl, status);
}

static void chat_tick(lv_timer_t * timer)
{
    LV_UNUSED(timer);
    debug_fields_once();
    chat_poll();

    /* Trust/REMOVE in the Contacts tab changed the trust list: the rows
       already on screen have to be redrawn, because their mark is what
       that change is about */
    if(contacts_version() != g_ui.c_ver) rebuild();
    refresh();

    /* test hook: say something on startup (PDA_TEST_SEND). PDA_TEST_SEND_IN
       holds the message back for that many seconds - the mesh test needs
       the device to have found its peer first. */
    static bool sent = false;
    if(!sent) {
        const char * t = getenv("PDA_TEST_SEND");
        if(t && *t) {
            static time_t t0 = 0;
            if(!t0) t0 = time(NULL);
            const char * d = getenv("PDA_TEST_SEND_IN");
            int wait = d && *d ? atoi(d) : 0;
            if(wait <= 0 || time(NULL) - t0 >= wait) {
                sent = true;
                chat_send(t);
            }
        }
    }
}

static void do_send(void)
{
    if(!g_ui.input) return;
    const char * txt = lv_textarea_get_text(g_ui.input);
    if(!txt || !*txt) return;
    char buf[CHAT_TEXT_MAX];
    snprintf(buf, sizeof(buf), "%s", txt);
    lv_textarea_set_text(g_ui.input, "");
    chat_send(buf);
}

static void on_input_ready(lv_event_t * e)
{
    LV_UNUSED(e);
    do_send();
}

static void on_send_click(lv_event_t * e)
{
    LV_UNUSED(e);
    do_send();
}

/* ---------- which carriers a message goes out on ---------- */

/* the ARGB value of a palette colour, for the little canvas */
static uint32_t pick_argb(uint32_t rgb)
{
    return 0xFF000000u | (rgb & 0xFFFFFFu);
}

/* the little disc in the button: the same as the markers of the map's layer
   strip - a filled disc with a smaller dot in it, big when the carrier is
   used and small when it is not */
static void draw_pick_disc(lv_obj_t * canvas, uint32_t * buf, bool on)
{
    int cx = DISC_N / 2, cy = DISC_N / 2;
    memset(buf, 0, sizeof(uint32_t) * (size_t)DISC_N * DISC_N);

    for(int j = cy - 5; j <= cy + 5; j++) {
        for(int i = cx - 5; i <= cx + 5; i++) {
            int dx = i - cx, dy = j - cy;
            if(dx * dx + dy * dy <= 25) buf[j * DISC_N + i] = pick_argb(PDA_SUB_DISC);
        }
    }
    int r = on ? 3 : 2;
    for(int j = cy - r; j <= cy + r; j++) {
        for(int i = cx - r; i <= cx + r; i++) {
            int dx = i - cx, dy = j - cy;
            if(dx * dx + dy * dy <= r * r)
                buf[j * DISC_N + i] = pick_argb(on ? PDA_SUB_DOT_ON : PDA_SUB_DOT_OFF);
        }
    }
    if(canvas) lv_obj_invalidate(canvas);
}

/* the setting a carrier is remembered under ("send_bit", "send_fips", ...) */
static void pick_key(int idx, char * out, size_t outn)
{
    const chat_transport_t * tr = chat_transport_get(idx);
    char tag[16] = "";
    if(tr) snprintf(tag, sizeof(tag), "%s", tr->tag);
    for(char * p = tag; *p; p++)
        if(*p >= 'A' && *p <= 'Z') *p = (char)(*p - 'A' + 'a');
    snprintf(out, outn, "send_%s", tag);
}

/* what that setting says right now. The internet carrier's key moved with
   its rename (NET -> nstr, so "send_net" -> "send_nstr"): a saved "send_net
   = 0" still holds this carrier off when "send_nstr" is absent - one line
   of fallback, no migration pass over the file. */
static bool pick_on(int idx)
{
    char key[24];
    pick_key(idx, key, sizeof(key));
    if(!prefs_str(key, NULL) && strcmp(key, "send_nstr") == 0)
        return prefs_bool("send_net", true);
    return prefs_bool(key, true);
}

/* the buttons: the disc says whether the carrier is used and the tag is
   bright or dim with it - the same look as the layer strip of the map */
static void restyle_pick(void)
{
    for(int i = 0; i < CHAT_TRANSPORT_MAX; i++) {
        if(!g_ui.pick_btn[i]) continue;
        bool on = chat_carrier_enabled(i);
        if(g_ui.pick_lbl[i]) {
            lv_obj_set_style_text_color(g_ui.pick_lbl[i],
                                        lv_color_hex(on ? PDA_SUB_TEXT_ON : PDA_TAB_TEXT),
                                        LV_PART_MAIN);
        }
        if(g_ui.pick_disc[i]) draw_pick_disc(g_ui.pick_disc[i], g_ui.pick_buf[i], on);
    }
}

static void on_pick_click(lv_event_t * e)
{
    int i = (int)(lv_intptr_t)lv_event_get_user_data(e);
    if(i < 0 || i >= CHAT_TRANSPORT_MAX) return;
    bool on = !chat_carrier_enabled(i);
    chat_set_carrier_enabled(i, on);
    char key[24];
    pick_key(i, key, sizeof(key));
    prefs_set_bool(key, on);
    restyle_pick();
    if(getenv("PDA_DEBUG")) {
        const chat_transport_t * tr = chat_transport_get(i);
        fprintf(stderr, "[chat] sending on %s: %s\n", tr ? tr->tag : "?", on ? "yes" : "no");
    }
}

/* the buttons in the strip: one per carrier, in the space between the
   status and the badges */
static void build_pick(lv_obj_t * strip, lv_group_t * group)
{
    int n = chat_transport_count();
    if(n <= 0) return;

    lv_obj_t * lbl = lv_label_create(strip);
    lv_label_set_text(lbl, "SEND ON");
    lv_obj_set_style_text_font(lbl, &pda_font_mono_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl, lv_color_hex(PDA_MSG_SYS), LV_PART_MAIN);
    lv_obj_set_style_pad_all(lbl, 0, LV_PART_MAIN);
    lv_obj_set_pos(lbl, 0, 0);       /* the place is set below */

    int x = PICK_RIGHT - n * PICK_W - (n - 1) * PICK_GAP;
    for(int i = 0; i < n && i < CHAT_TRANSPORT_MAX; i++) {
        const chat_transport_t * tr = chat_transport_get(i);
        if(!tr) continue;

        lv_obj_t * b = lv_button_create(strip);
        lv_obj_set_pos(b, x + i * (PICK_W + PICK_GAP), 4);
        lv_obj_set_size(b, PICK_W, PICK_H);
        lv_obj_set_style_bg_color(b, lv_color_hex(PDA_SUB_FILL), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(b, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(b, 1, LV_PART_MAIN);
        lv_obj_set_style_border_color(b, lv_color_hex(PDA_SUB_EDGE), LV_PART_MAIN);
        lv_obj_set_style_radius(b, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(b, 0, LV_PART_MAIN);
        lv_no_transition(b);

        /* the little disc, like the markers of the map's layer strip */
        lv_obj_t * d = lv_canvas_create(b);
        lv_canvas_set_buffer(d, g_ui.pick_buf[i], DISC_N, DISC_N,
                              LV_COLOR_FORMAT_ARGB8888);
        lv_obj_set_pos(d, 3, 3);
        lv_obj_set_size(d, DISC_N, DISC_N);
        lv_obj_set_style_border_width(d, 0, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(d, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_pad_all(d, 0, LV_PART_MAIN);
        lv_obj_set_clickable(d, false);

        lv_obj_t * l = lv_label_create(b);
        lv_label_set_text(l, tr->tag);
        lv_obj_set_pos(l, 14, 4);
        lv_obj_set_width(l, PICK_W - 16);
        lv_label_set_long_mode(l, LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_set_style_text_font(l, &pda_font_mono_10, LV_PART_MAIN);
        lv_obj_set_style_pad_all(l, 0, LV_PART_MAIN);

        lv_obj_add_event_cb(b, on_pick_click, LV_EVENT_CLICKED, (void *)(lv_intptr_t)i);
        g_ui.pick_btn[i] = b;
        g_ui.pick_lbl[i] = l;
        g_ui.pick_disc[i] = d;
        if(group) lv_group_add_obj(group, b);
    }

    /* the "SEND ON" label sits to the left of the buttons */
    lv_point_t sz;
    lv_text_get_size(&sz, "SEND ON", &pda_font_mono_10, 0, 0, LV_COORD_MAX,
                     LV_TEXT_ALIGN_LEFT);
    lv_obj_set_pos(lbl, x - sz.x - 8, 8);
    restyle_pick();
}

lv_obj_t * chat_ui_create(lv_obj_t * parent, lv_group_t * group)
{
    memset(&g_ui, 0, sizeof(g_ui));

    /* which carriers a message goes out on - the setting is remembered */
    int ntr = chat_transport_count();
    for(int i = 0; i < ntr && i < CHAT_TRANSPORT_MAX; i++) {
        chat_set_carrier_enabled(i, pick_on(i));
    }

    lv_obj_t * root = lv_obj_create(parent);
    lv_obj_set_pos(root, 0, 0);
    lv_obj_set_size(root, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(root, lv_color_hex(PDA_CHAT_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(root, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(root, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_row(root, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(root, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(root, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(root, LV_DIR_NONE);
    lv_obj_set_scrollable(root, false);
    g_ui.root = root;

    /* --- the gritty surface everything sits on --- */
    grit_backdrop(root, BACK_W, BACK_H);

    /* --- the carrier strip --- */
    lv_obj_t * strip = lv_obj_create(root);
    lv_obj_set_pos(strip, 0, 0);
    lv_obj_set_size(strip, LIST_W, STRIP_H);
    lv_obj_set_style_bg_opa(strip, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(strip, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(strip, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(strip, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(strip, LV_DIR_NONE);
    lv_obj_set_scrollable(strip, false);

    /* the little square indicator of the guide screens */
    grit_square(strip, 8, 9, 8, PDA_MSG_ACCENT);

    g_ui.room_lbl = lv_label_create(strip);
    lv_label_set_text(g_ui.room_lbl, chat_room());
    lv_obj_set_pos(g_ui.room_lbl, 24, 5);
    lv_obj_set_style_text_font(g_ui.room_lbl, &pda_font_mono_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(g_ui.room_lbl, lv_color_hex(PDA_MSG_ACCENT), LV_PART_MAIN);

    g_ui.status_lbl = lv_label_create(strip);
    lv_label_set_text(g_ui.status_lbl, "");
    lv_obj_set_pos(g_ui.status_lbl, 92, 7);
    lv_obj_set_width(g_ui.status_lbl, 300);
    lv_label_set_long_mode(g_ui.status_lbl, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(g_ui.status_lbl, &pda_font_mono_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(g_ui.status_lbl, lv_color_hex(PDA_MSG_SYS), LV_PART_MAIN);

    /* the carrier badges, right aligned */
    int n = chat_transport_count();
    int bx = 786;
    for(int i = n - 1; i >= 0; i--) {
        const chat_transport_t * tr = chat_transport_get(i);
        if(!tr) continue;
        lv_obj_t * b = lv_label_create(strip);
        lv_label_set_text(b, tr->tag);
        lv_obj_set_width(b, 44);
        lv_obj_set_style_text_align(b, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
        lv_obj_set_style_text_font(b, &pda_font_mono_10, LV_PART_MAIN);
        lv_obj_set_style_text_color(b, lv_color_hex(PDA_CARRIER_DOWN), LV_PART_MAIN);
        lv_obj_set_pos(b, bx - 44, 7);
        bx -= 48;
        g_ui.badge[i] = b;
    }

    /* the buttons that say which carriers a message goes out on - they sit
       in the space between the status and the badges */
    build_pick(strip, group);

    /* the rule under the strip */
    grit_rule(root, 6, STRIP_H + 2, 788, false, PDA_RULE);

    /* --- the frame: the log and the compose row live in it --- */
    lv_obj_t * frame = grit_frame(root, FRAME_X, FRAME_Y, FRAME_W, FRAME_H, PDA_FRAME);
    if(frame) {
        lv_obj_set_style_bg_color(frame, lv_color_hex(PDA_PAGE_BG), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(frame, LV_OPA_90, LV_PART_MAIN);
    }
    lv_obj_t * panel = frame ? frame : root;

    /* --- the log --- */
    lv_obj_t * list = lv_obj_create(panel);
    lv_obj_set_pos(list, 8, 8);
    lv_obj_set_size(list, FRAME_W - 16, FRAME_H - 16 - COMPOSE_H);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(list, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(list, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_row(list, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(list, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);
    lv_obj_set_scrollable(list, true);
    lv_obj_set_style_bg_color(list, lv_color_hex(PDA_SCROLL), LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_opa(list, LV_OPA_COVER, LV_PART_SCROLLBAR);
    lv_obj_set_style_width(list, 4, LV_PART_SCROLLBAR);
    lv_obj_set_style_radius(list, 0, LV_PART_SCROLLBAR);
    g_ui.list = list;

    /* --- the compose row --- */
    lv_obj_t * compose = lv_obj_create(panel);
    lv_obj_set_pos(compose, 8, FRAME_H - 8 - COMPOSE_H);
    lv_obj_set_size(compose, FRAME_W - 16, COMPOSE_H);
    lv_obj_set_style_bg_opa(compose, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(compose, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(compose, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(compose, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(compose, LV_DIR_NONE);
    lv_obj_set_scrollable(compose, false);

    /* the rule between the log and the compose row */
    grit_rule(compose, 0, 0, lv_pct(100), false, PDA_RULE);

    lv_obj_t * ta = lv_textarea_create(compose);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_placeholder_text(ta, tr(TR_TYPE_MESSAGE));
    lv_obj_set_pos(ta, 4, 6);
    lv_obj_set_size(ta, 620, 26);
    /* the field must not shrink to the size of its text */
    lv_obj_set_style_min_width(ta, 620, LV_PART_MAIN);
    lv_obj_set_style_bg_color(ta, lv_color_hex(PDA_CHAT_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ta, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(ta, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(ta, lv_color_hex(PDA_CHAT_EDGE), LV_PART_MAIN);
    lv_obj_set_style_text_font(ta, &pda_font_mono_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(ta, lv_color_hex(PDA_MSG_TEXT), LV_PART_MAIN);
    lv_obj_set_style_text_color(ta, lv_color_hex(PDA_MSG_SYS), LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_pad_all(ta, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_left(ta, 6, LV_PART_MAIN);
    /* the cursor only shows while the field has the focus */
    lv_caret_init(ta);
    g_ui.input = ta;
    lv_obj_add_event_cb(ta, on_input_ready, LV_EVENT_READY, NULL);
    if(group) lv_group_add_obj(group, ta);

    lv_obj_t * send = lv_button_create(compose);
    lv_obj_set_pos(send, 636, 5);
    lv_obj_set_size(send, 112, 28);
    lv_obj_set_style_bg_color(send, lv_color_hex(PDA_CHAT_EDGE), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(send, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(send, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(send, lv_color_hex(PDA_MSG_ACCENT), LV_PART_MAIN);
    lv_obj_set_style_radius(send, 0, LV_PART_MAIN);
    lv_no_transition(send);
    lv_obj_t * slbl = lv_label_create(send);
    lv_label_set_text(slbl, tr(TR_SEND));
    lv_obj_set_style_text_font(slbl, &pda_font_mono_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(slbl, lv_color_hex(PDA_MSG_ACCENT), LV_PART_MAIN);
    lv_obj_center(slbl);
    lv_obj_add_event_cb(send, on_send_click, LV_EVENT_CLICKED, NULL);

    /* the crop marks go on last, so they are on top of the panel */
    if(frame) grit_corners(frame, FRAME_W, FRAME_H, PDA_FRAME);

    g_ui.c_ver = contacts_version();
    refresh();
    int poll = atoi(prefs_str("log_poll", "200"));
    if(poll < 50) poll = 200;
    g_ui.timer = lv_timer_create(chat_tick, poll, NULL);

    g_ui.send_btn = send;
    return root;
}
