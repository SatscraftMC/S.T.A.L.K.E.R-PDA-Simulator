#include "ui/chat_ui.h"
#include "ui/palette.h"
#include "assets/fonts/pda_fonts.h"
#include "chat/chat.h"

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
 *   +-------------------------------------------------------------+
 *   | SECTOR   mesh 2425 / relay 1.2.3.4:7788     BIT  FIPS  NET  |
 *   +-------------------------------------------------------------+
 *   | 12:04  ROOK     anyone got the drop coords?                 |
 *   | 12:05  ME       not yet, rain again                        |
 *   +-------------------------------------------------------------+
 *   | [ type a message...                                ] [ SEND ] |
 *   +-------------------------------------------------------------+
 */

#define LIST_W        800
#define STRIP_H       26
#define COMPOSE_H     38
#define ROW_MAX       120
#define TEXT_X        152
#define TEXT_W        640
#define NAME_X        44
#define NAME_W        62
#define TAG_X         108
#define TAG_W         42

typedef struct {
    lv_obj_t * root;
    lv_obj_t * list;
    lv_obj_t * room_lbl;
    lv_obj_t * status_lbl;
    lv_obj_t * badge[CHAT_TRANSPORT_MAX];
    lv_obj_t * input;
    int rendered;         /* messages already drawn */
    int rows;              /* rows currently in the list */
    int y;                 /* running y of the next row */
    int row_h[ROW_MAX];    /* height of each row, for the scroll-back */
} chat_ui_t;

static chat_ui_t g_ui;

static void chat_tick(lv_timer_t * timer);

static void scroll_to_bottom(void)
{
    lv_obj_scroll_to_view(g_ui.list, LV_ANIM_OFF);
}

static void add_row(const chat_msg_t * m)
{
    char tbuf[8];
    struct tm lt;
    time_t t = m->ts;
    localtime_r(&t, &lt);
    strftime(tbuf, sizeof(tbuf), "%H:%M", &lt);

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

    if(g_ui.rows >= ROW_MAX) {
        /* drop the oldest row and pull everything up */
        int drop = g_ui.row_h[0];
        for(int i = 0; i < g_ui.rows - 1; i++) g_ui.row_h[i] = g_ui.row_h[i + 1];
        g_ui.rows--;
        g_ui.y -= drop;
        lv_obj_t * child = lv_obj_get_child(g_ui.list, 0);
        if(child) lv_obj_delete(child);
        for(int i = 0; i < g_ui.rows; i++) {
            lv_obj_t * c = lv_obj_get_child(g_ui.list, i);
            if(c) lv_obj_set_pos(c, 0, g_ui.y - (g_ui.rows - i) * 0 - 0);
        }
        /* reposition: rows keep their order, just shifted up by the dropped height */
        int yy = 0;
        for(int i = 0; i < g_ui.rows; i++) {
            lv_obj_t * c = lv_obj_get_child(g_ui.list, i);
            if(c) lv_obj_set_pos(c, 0, yy);
            yy += g_ui.row_h[i];
        }
        g_ui.y = yy;
    }

    lv_obj_t * row = lv_obj_create(g_ui.list);
    lv_obj_set_pos(row, 0, g_ui.y);
    lv_obj_set_size(row, LIST_W, 20);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(row, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(row, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(row, LV_DIR_NONE);
    lv_obj_set_scrollable(row, false);
    lv_obj_set_clickable(row, false);

    lv_obj_t * tl = lv_label_create(row);
    lv_label_set_text(tl, tbuf);
    lv_obj_set_pos(tl, 4, 1);
    lv_obj_set_width(tl, 38);
    lv_obj_set_style_text_font(tl, &pda_font_mono_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(tl, lv_color_hex(PDA_MSG_TIME), LV_PART_MAIN);
    lv_obj_set_style_pad_all(tl, 0, LV_PART_MAIN);

    lv_obj_t * nl = lv_label_create(row);
    lv_label_set_text(nl, m->sender);
    lv_obj_set_pos(nl, NAME_X, 1);
    lv_obj_set_width(nl, NAME_W);
    lv_label_set_long_mode(nl, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(nl, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_obj_set_style_text_font(nl, nfont, LV_PART_MAIN);
    lv_obj_set_style_text_color(nl, lv_color_hex(name_col), LV_PART_MAIN);
    lv_obj_set_style_pad_all(nl, 0, LV_PART_MAIN);

    /* the carrier it came in on */
    if(m->transport >= 0 && m->transport < chat_transport_count()) {
        const chat_transport_t * tr = chat_transport_get(m->transport);
        if(tr) {
            lv_obj_t * cl = lv_label_create(row);
            lv_label_set_text(cl, tr->tag);
            lv_obj_set_pos(cl, TAG_X, 2);
            lv_obj_set_width(cl, TAG_W);
            lv_obj_set_style_text_font(cl, &pda_font_mono_10, LV_PART_MAIN);
            lv_obj_set_style_text_color(cl, lv_color_hex(PDA_CARRIER_UP), LV_PART_MAIN);
            lv_obj_set_style_pad_all(cl, 0, LV_PART_MAIN);
        }
    }

    /* the label lays the text out itself: let it, and take its height */
    lv_obj_t * xl = lv_label_create(row);
    lv_label_set_text(xl, text);
    lv_obj_set_pos(xl, TEXT_X, 0);
    lv_obj_set_width(xl, TEXT_W);
    lv_label_set_long_mode(xl, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_pad_all(xl, 0, LV_PART_MAIN);
    lv_obj_set_style_text_font(xl, &pda_font_mono_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(xl, lv_color_hex(PDA_MSG_TEXT), LV_PART_MAIN);
    lv_obj_update_layout(xl);
    int th = lv_obj_get_height(xl);
    if(th <= 0) th = 14;
    lv_obj_set_height(row, th + 6);

    g_ui.row_h[g_ui.rows] = th + 6;
    g_ui.rows++;
    g_ui.y += th + 6;
}

static void refresh(void)
{
    int count = chat_count();
    if(count < g_ui.rendered) {     /* the log was trimmed: rebuild */
        lv_obj_clean(g_ui.list);
        g_ui.rows = 0;
        g_ui.y = 0;
        g_ui.rendered = 0;
    }
    for(int i = g_ui.rendered; i < count; i++) {
        const chat_msg_t * m = chat_get(i);
        if(m) add_row(m);
    }
    g_ui.rendered = count;

    /* the carrier strip */
    char status[128] = "";
    int n = chat_transport_count();
    for(int i = 0; i < n && i < CHAT_TRANSPORT_MAX; i++) {
        const chat_transport_t * tr = chat_transport_get(i);
        if(!tr) continue;
        uint32_t col = tr->up ? PDA_CARRIER_UP : PDA_CARRIER_DOWN;
        if(g_ui.badge[i]) {
            lv_obj_set_style_text_color(g_ui.badge[i], lv_color_hex(col), LV_PART_MAIN);
        }
        if(tr->up && tr->status[0]) {
            if(status[0]) strncat(status, "  /  ", sizeof(status) - strlen(status) - 1);
            strncat(status, tr->status, sizeof(status) - strlen(status) - 1);
        }
    }
    if(g_ui.status_lbl) lv_label_set_text(g_ui.status_lbl, status);
}

static void chat_tick(lv_timer_t * timer)
{
    LV_UNUSED(timer);
    chat_poll();
    refresh();

    /* test hook: say something on startup (PDA_TEST_SEND) */
    static bool sent = false;
    if(!sent) {
        const char * t = getenv("PDA_TEST_SEND");
        if(t && *t) {
            sent = true;
            chat_send(t);
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

lv_obj_t * chat_ui_create(lv_obj_t * parent, lv_group_t * group)
{
    memset(&g_ui, 0, sizeof(g_ui));

    lv_obj_t * root = lv_obj_create(parent);
    lv_obj_set_pos(root, 0, 0);
    lv_obj_set_size(root, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(root, lv_color_hex(PDA_CHAT_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(root, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(root, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(root, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(root, LV_DIR_NONE);
    lv_obj_set_scrollable(root, false);
    g_ui.root = root;

    /* --- the carrier strip --- */
    lv_obj_t * strip = lv_obj_create(root);
    lv_obj_set_pos(strip, 0, 0);
    lv_obj_set_size(strip, LIST_W, STRIP_H);
    lv_obj_set_style_bg_color(strip, lv_color_hex(PDA_CHAT_STRIP), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(strip, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(strip, 0, LV_PART_MAIN);
    lv_obj_set_style_border_side(strip, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
    lv_obj_set_style_border_color(strip, lv_color_hex(PDA_CHAT_EDGE), LV_PART_MAIN);
    lv_obj_set_style_border_width(strip, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(strip, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(strip, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(strip, LV_DIR_NONE);
    lv_obj_set_scrollable(strip, false);

    g_ui.room_lbl = lv_label_create(strip);
    lv_label_set_text(g_ui.room_lbl, chat_room());
    lv_obj_set_pos(g_ui.room_lbl, 8, 5);
    lv_obj_set_style_text_font(g_ui.room_lbl, &pda_font_mono_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(g_ui.room_lbl, lv_color_hex(PDA_MSG_ACCENT), LV_PART_MAIN);

    g_ui.status_lbl = lv_label_create(strip);
    lv_label_set_text(g_ui.status_lbl, "");
    lv_obj_set_pos(g_ui.status_lbl, 92, 7);
    lv_obj_set_width(g_ui.status_lbl, 460);
    lv_label_set_long_mode(g_ui.status_lbl, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(g_ui.status_lbl, &pda_font_mono_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(g_ui.status_lbl, lv_color_hex(PDA_MSG_SYS), LV_PART_MAIN);

    /* the carrier badges, right aligned */
    int n = chat_transport_count();
    int bx = LIST_W - 10;
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

    /* --- the log --- */
    lv_obj_t * list = lv_obj_create(root);
    lv_obj_set_pos(list, 0, STRIP_H);
    lv_obj_set_size(list, LIST_W, 350);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(list, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(list, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_row(list, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(list, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);
    lv_obj_set_scrollable(list, true);
    lv_obj_set_style_bg_color(list, lv_color_hex(PDA_CHAT_EDGE), LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_opa(list, LV_OPA_50, LV_PART_SCROLLBAR);
    lv_obj_set_style_width(list, 3, LV_PART_SCROLLBAR);
    g_ui.list = list;

    /* --- the compose row --- */
    lv_obj_t * compose = lv_obj_create(root);
    lv_obj_set_pos(compose, 0, 350);
    lv_obj_set_size(compose, LIST_W, COMPOSE_H);
    lv_obj_set_style_bg_color(compose, lv_color_hex(PDA_CHAT_INPUT), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(compose, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(compose, 0, LV_PART_MAIN);
    lv_obj_set_style_border_side(compose, LV_BORDER_SIDE_TOP, LV_PART_MAIN);
    lv_obj_set_style_border_color(compose, lv_color_hex(PDA_CHAT_EDGE), LV_PART_MAIN);
    lv_obj_set_style_border_width(compose, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(compose, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(compose, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(compose, LV_DIR_NONE);
    lv_obj_set_scrollable(compose, false);

    lv_obj_t * ta = lv_textarea_create(compose);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_placeholder_text(ta, "type a message");
    lv_obj_set_pos(ta, 8, 6);
    lv_obj_set_size(ta, 660, 26);
    lv_obj_set_style_bg_color(ta, lv_color_hex(PDA_CHAT_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ta, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(ta, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(ta, lv_color_hex(PDA_CHAT_EDGE), LV_PART_MAIN);
    lv_obj_set_style_text_font(ta, &pda_font_mono_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(ta, lv_color_hex(PDA_MSG_TEXT), LV_PART_MAIN);
    lv_obj_set_style_text_color(ta, lv_color_hex(PDA_MSG_SYS), LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_border_width(ta, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(ta, 4, LV_PART_MAIN);
    g_ui.input = ta;
    lv_obj_add_event_cb(ta, on_input_ready, LV_EVENT_READY, NULL);
    if(group) lv_group_add_obj(group, ta);

    lv_obj_t * send = lv_button_create(compose);
    lv_obj_set_pos(send, 678, 5);
    lv_obj_set_size(send, 112, 28);
    lv_obj_set_style_bg_color(send, lv_color_hex(PDA_CHAT_EDGE), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(send, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(send, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(send, lv_color_hex(PDA_MSG_ACCENT), LV_PART_MAIN);
    lv_obj_set_style_radius(send, 0, LV_PART_MAIN);
    lv_obj_t * slbl = lv_label_create(send);
    lv_label_set_text(slbl, "SEND");
    lv_obj_set_style_text_font(slbl, &pda_font_mono_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(slbl, lv_color_hex(PDA_MSG_ACCENT), LV_PART_MAIN);
    lv_obj_center(slbl);
    lv_obj_add_event_cb(send, on_send_click, LV_EVENT_CLICKED, NULL);

    refresh();
    lv_timer_create(chat_tick, 200, NULL);
    return root;
}
