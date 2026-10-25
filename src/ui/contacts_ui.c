#include "ui/contacts_ui.h"
#include "ui/palette.h"
#include "ui/grit.h"
#include "ui/i18n.h"
#include "ui/lvutil.h"
#include "assets/fonts/pda_fonts.h"
#include "chat/chat.h"
#include "identity/contacts.h"
#include "identity/identity.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The Contacts tab: SEEN under TRUSTED, both headings always there, one
   scrollable column (up to CONTACT_MAX 32 saved keys plus the session's
   speakers do not fit on one screen).

   The split is a render-time filter with one rule: TRUSTED lists
   contacts_at(i), SEEN lists speakers where !contacts_has(pub) - noting a
   speaker happens regardless of trust, and the filter decides where the
   row appears. [TRUST] lives only on SEEN rows, [REMOVE] only on TRUSTED.

   Both buttons are real: they write or delete contacts.conf, and only a
   successful contacts_save() moves the row (contacts.c is file-first, so
   a failure leaves the row put, flashes FAILED here, and writes a SYSTEM
   line - never a fake-working control, never an entry that vanishes at
   restart). */

#define STRIP_H   26
#define FRAME_X   6
#define FRAME_Y   32
#define FRAME_W   788
#define FRAME_H   407
#define LIST_PAD  8
#define ROW_H     22
#define ROW_H_OPEN 38          /* a row showing its full npub */
#define NAME_X    6
#define NAME_W    180
#define PUB_X     196
#define PUB_W     240
#define BTN_X     684
#define BTN_W     76
#define BTN_H     16
#define DETAIL_Y  20
#define DETAIL_W  740
#define MAX_ROWS  (CONTACT_MAX + 24)

typedef struct {
    lv_obj_t * lbl;
    char restore[16];
    lv_timer_t * timer;
} flash_t;

typedef struct {
    lv_obj_t * root;
    lv_obj_t * list;
    lv_obj_t * title;
    lv_timer_t * tick;
    lv_group_t * group;

    unsigned c_ver;                 /* what the last rebuild saw */
    unsigned s_ver;

    char expanded[65];              /* the row whose npub detail is open */
    char focus_pub[65];             /* where the keypad was before a rebuild */
    bool restore_focus;

    char row_pub[MAX_ROWS][65];     /* the pubs of the rows on screen (a
                                       click reads the pool, never a live
                                       pointer into a list that may move) */
    lv_obj_t * row_obj[MAX_ROWS];
    lv_obj_t * btn_lbl[MAX_ROWS];
    int n_rows;

    flash_t flash[4];
} contacts_ui_t;

static contacts_ui_t g_ui;

static void rebuild(void);

/* ---------- the FAILED flash on a button that could not do its job ---------- */

static void flash_cb(lv_timer_t * timer)
{
    int i = (int)(lv_intptr_t)lv_timer_get_user_data(timer);
    if(i >= 0 && i < 4) {
        flash_t * f = &g_ui.flash[i];
        f->timer = NULL;
        if(f->lbl) {
            lv_label_set_text(f->lbl, f->restore);
            lv_obj_set_style_text_color(f->lbl, lv_color_hex(PDA_MSG_ACCENT),
                                        LV_PART_MAIN);
        }
    }
    lv_timer_delete(timer);
}

static void cancel_flashes(void)
{
    for(int i = 0; i < 4; i++) {
        if(g_ui.flash[i].timer) {
            lv_timer_delete(g_ui.flash[i].timer);
            g_ui.flash[i].timer = NULL;
        }
        g_ui.flash[i].lbl = NULL;
    }
}

static void flash_failed(lv_obj_t * lbl, const char * restore)
{
    if(!lbl) return;
    for(int i = 0; i < 4; i++) {
        if(g_ui.flash[i].timer) continue;
        lv_label_set_text(lbl, tr(TR_FAILED));
        lv_obj_set_style_text_color(lbl, lv_color_hex(PDA_CARRIER_DOWN), LV_PART_MAIN);
        g_ui.flash[i].lbl = lbl;
        snprintf(g_ui.flash[i].restore, sizeof(g_ui.flash[i].restore), "%s", restore);
        g_ui.flash[i].timer = lv_timer_create(flash_cb, 1500, (void *)(lv_intptr_t)i);
        return;
    }
}

/* ---------- the rows ---------- */

static void heading(const char * text)
{
    lv_obj_t * h = lv_obj_create(g_ui.list);
    lv_obj_set_size(h, lv_pct(100), 16);
    lv_obj_set_flex_grow(h, 0);
    lv_obj_set_style_bg_opa(h, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(h, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(h, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(h, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(h, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(h, LV_DIR_NONE);
    lv_obj_set_clickable(h, false);

    lv_obj_t * sq = lv_obj_create(h);
    lv_obj_set_pos(sq, 0, 5);
    lv_obj_set_size(sq, 6, 6);
    lv_obj_set_style_bg_color(sq, lv_color_hex(PDA_MSG_ACCENT), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(sq, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(sq, 0, LV_PART_MAIN);

    lv_obj_t * l = lv_label_create(h);
    lv_label_set_text(l, text);
    lv_obj_set_pos(l, 12, 1);
    lv_obj_set_style_text_font(l, &pda_font_mono_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(l, lv_color_hex(PDA_MSG_ACCENT), LV_PART_MAIN);

    grit_rule(h, 0, 15, lv_pct(100), false, PDA_RULE);
}

static void empty_line(const char * text)
{
    lv_obj_t * l = lv_label_create(g_ui.list);
    lv_label_set_text(l, text);
    lv_obj_set_size(l, lv_pct(100), 16);
    lv_obj_set_flex_grow(l, 0);
    lv_obj_set_style_pad_left(l, 12, LV_PART_MAIN);
    lv_obj_set_style_text_font(l, &pda_font_mono_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(l, lv_color_hex(PDA_MSG_SYS), LV_PART_MAIN);
}

static int slot_of(lv_obj_t * o)
{
    lv_intptr_t v = (lv_intptr_t)lv_obj_get_user_data(o);
    return (int)v - 1;                /* rows/buttons store slot + 1 */
}

static const char * speaker_name(const char * pub)
{
    int n = chat_speaker_count();
    for(int i = 0; i < n; i++) {
        const chat_speaker_t * s = chat_speaker_at(i);
        if(s && strcmp(s->pub, pub) == 0 && s->name[0]) return s->name;
    }
    return "UNKNOWN";
}

static void add_row(const char * pub, const char * name, bool trusted);

static void on_row_click(lv_event_t * e)
{
    int slot = slot_of(lv_event_get_current_target(e));
    if(slot < 0 || slot >= g_ui.n_rows) return;
    if(strcmp(g_ui.expanded, g_ui.row_pub[slot]) == 0) g_ui.expanded[0] = 0;
    else snprintf(g_ui.expanded, sizeof(g_ui.expanded), "%s", g_ui.row_pub[slot]);
    rebuild();
}

static void on_trust_click(lv_event_t * e)
{
    lv_obj_t * btn = lv_event_get_current_target(e);
    int slot = slot_of(btn);
    if(slot < 0 || slot >= g_ui.n_rows) return;
    const char * pub = g_ui.row_pub[slot];
    lv_obj_t * lbl = lv_obj_get_child(btn, 0);

    if(!contacts_trust(pub, speaker_name(pub))) {
        /* the row stays in SEEN: file-first means no move without a save */
        flash_failed(lbl, tr(TR_CONTACT_TRUST));
        if(getenv("PDA_DEBUG"))
            fprintf(stderr, "[contacts] trust failed for %s\n", pub);
    }
    /* on success contacts_version() has already bumped: the next tick
       moves the row to TRUSTED on its own */
}

static void on_remove_click(lv_event_t * e)
{
    lv_obj_t * btn = lv_event_get_current_target(e);
    int slot = slot_of(btn);
    if(slot < 0 || slot >= g_ui.n_rows) return;
    const char * pub = g_ui.row_pub[slot];
    lv_obj_t * lbl = lv_obj_get_child(btn, 0);

    if(!contacts_untrust(pub)) {
        flash_failed(lbl, tr(TR_CONTACT_REMOVE));
        if(getenv("PDA_DEBUG"))
            fprintf(stderr, "[contacts] remove failed for %s\n", pub);
    }
}

static void add_row(const char * pub, const char * name, bool trusted)
{
    int slot = g_ui.n_rows;
    if(slot >= MAX_ROWS) return;
    snprintf(g_ui.row_pub[slot], sizeof(g_ui.row_pub[slot]), "%s", pub);
    g_ui.n_rows++;

    bool open = (strcmp(g_ui.expanded, pub) == 0);

    lv_obj_t * row = lv_obj_create(g_ui.list);
    lv_obj_set_size(row, lv_pct(100), open ? ROW_H_OPEN : ROW_H);
    lv_obj_set_flex_grow(row, 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(row, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(row, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(row, LV_DIR_NONE);
    lv_obj_set_scrollable(row, false);
    lv_obj_set_clickable(row, true);
    lv_obj_set_user_data(row, (void *)(lv_intptr_t)(slot + 1));
    lv_obj_add_event_cb(row, on_row_click, LV_EVENT_CLICKED,
                        (void *)(lv_intptr_t)(slot + 1));
    if(g_ui.group) lv_group_add_obj(g_ui.group, row);
    g_ui.row_obj[slot] = row;

    lv_obj_t * nl = lv_label_create(row);
    lv_label_set_text(nl, (name && *name) ? name : "UNKNOWN");
    lv_obj_set_pos(nl, NAME_X, 3);
    lv_obj_set_width(nl, NAME_W);
    lv_label_set_long_mode(nl, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(nl, &pda_font_mono_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(nl, lv_color_hex(PDA_MSG_TEXT), LV_PART_MAIN);

    lv_obj_t * pl = lv_label_create(row);
    lv_label_set_text(pl, pub);
    lv_obj_set_pos(pl, PUB_X, 3);
    lv_obj_set_width(pl, PUB_W);
    lv_label_set_long_mode(pl, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(pl, &pda_font_mono_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(pl, lv_color_hex(PDA_MSG_SYS), LV_PART_MAIN);

    lv_obj_t * b = lv_button_create(row);
    lv_obj_set_pos(b, BTN_X, 2);
    lv_obj_set_size(b, BTN_W, BTN_H);
    lv_obj_set_style_bg_color(b, lv_color_hex(PDA_CHAT_EDGE), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(b, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(b, lv_color_hex(PDA_MSG_ACCENT), LV_PART_MAIN);
    lv_obj_set_style_radius(b, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(b, 0, LV_PART_MAIN);
    lv_no_transition(b);
    lv_obj_set_user_data(b, (void *)(lv_intptr_t)(slot + 1));

    lv_obj_t * bl = lv_label_create(b);
    lv_label_set_text(bl, trusted ? tr(TR_CONTACT_REMOVE) : tr(TR_CONTACT_TRUST));
    lv_obj_set_style_text_font(bl, &pda_font_mono_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(bl, lv_color_hex(PDA_MSG_ACCENT), LV_PART_MAIN);
    lv_obj_center(bl);
    g_ui.btn_lbl[slot] = bl;
    lv_obj_add_event_cb(b, trusted ? on_remove_click : on_trust_click,
                        LV_EVENT_CLICKED, (void *)(lv_intptr_t)(slot + 1));
    if(g_ui.group) lv_group_add_obj(g_ui.group, b);

    if(open) {
        char npub[64] = "";
        if(!identity_npub_from_pub_hex(pub, npub, sizeof(npub)))
            snprintf(npub, sizeof(npub), "%s", pub);
        lv_obj_t * d = lv_label_create(row);
        lv_label_set_text(d, npub);
        lv_obj_set_pos(d, NAME_X, DETAIL_Y);
        lv_obj_set_width(d, DETAIL_W);
        lv_label_set_long_mode(d, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_font(d, &pda_font_mono_10, LV_PART_MAIN);
        lv_obj_set_style_text_color(d, lv_color_hex(PDA_MSG_SYS), LV_PART_MAIN);
    }

    grit_rule(row, 0, (open ? ROW_H_OPEN : ROW_H) - 1, lv_pct(100), false, PDA_RULE);
}

static void build_rows(void)
{
    g_ui.n_rows = 0;
    memset(g_ui.row_obj, 0, sizeof(g_ui.row_obj));
    memset(g_ui.btn_lbl, 0, sizeof(g_ui.btn_lbl));

    /* TRUSTED: file order, a stable list that does not shuffle under you */
    heading(tr(TR_CONTACTS_TRUSTED));
    int n = contacts_count();
    if(n == 0) empty_line(tr(TR_CONTACTS_NONE_TRUSTED));
    for(int i = 0; i < n; i++) {
        const char * pub = contacts_at(i);
        if(!pub) continue;
        const char * label = contacts_name(pub);
        add_row(pub, label ? label : "UNKNOWN", true);
    }

    /* SEEN: session speakers not yet trusted, most recent first */
    heading(tr(TR_CONTACTS_SEEN));
    int sp = chat_speaker_count();
    int order[CHAT_PEER_MAX];
    int m = 0;
    for(int i = 0; i < sp && m < CHAT_PEER_MAX; i++) {
        const chat_speaker_t * s = chat_speaker_at(i);
        if(s) order[m++] = i;
    }
    for(int i = 1; i < m; i++) {                 /* sort by last, newest */
        int key = order[i];
        time_t kt = chat_speaker_at(key)->last;
        int j = i - 1;
        while(j >= 0 && chat_speaker_at(order[j])->last < kt) {
            order[j + 1] = order[j];
            j--;
        }
        order[j + 1] = key;
    }
    int shown = 0;
    for(int i = 0; i < m; i++) {
        const chat_speaker_t * s = chat_speaker_at(order[i]);
        if(!s || contacts_has(s->pub)) continue;
        add_row(s->pub, s->name, false);
        shown++;
    }
    if(shown == 0) empty_line(tr(TR_CONTACTS_NONE_SEEN));
}

/* ---------- keep it live ---------- */

static bool ours(lv_obj_t * o)
{
    while(o) {
        if(o == g_ui.root) return true;
        o = lv_obj_get_parent(o);
    }
    return false;
}

static void rebuild(void)
{
    /* a pending FAILED flash points at a label about to be deleted */
    cancel_flashes();

    int old_y = g_ui.list ? lv_obj_get_scroll_y(g_ui.list) : 0;
    bool stick = (old_y <= 4);

    /* where the keypad was: only ours to put back */
    g_ui.restore_focus = false;
    g_ui.focus_pub[0] = 0;
    if(g_ui.group) {
        lv_obj_t * f = lv_group_get_focused(g_ui.group);
        if(f && ours(f)) {
            g_ui.restore_focus = true;
            int slot = slot_of(f);
            if(slot >= 0 && slot < g_ui.n_rows)
                snprintf(g_ui.focus_pub, sizeof(g_ui.focus_pub), "%s",
                         g_ui.row_pub[slot]);
        }
    }

    if(g_ui.list) {
        lv_kill_tree(g_ui.list);
        lv_obj_clean(g_ui.list);
    }
    build_rows();

    g_ui.c_ver = contacts_version();
    g_ui.s_ver = chat_speaker_version();

    if(g_ui.list) {
        /* the layout is recreated with the same rows: the previous offset
           is restored as-is (clamped), or 0 if the view was at the top -
           deliberately not refresh()'s old_y + added rule, which would
           fling a rebuild to the bottom */
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

    if(g_ui.restore_focus && g_ui.group && g_ui.n_rows > 0) {
        int target = -1;
        if(g_ui.focus_pub[0]) {
            for(int i = 0; i < g_ui.n_rows; i++) {
                if(strcmp(g_ui.row_pub[i], g_ui.focus_pub) == 0) { target = i; break; }
            }
        }
        if(target < 0) target = 0;
        if(g_ui.row_obj[target]) lv_group_focus_obj(g_ui.row_obj[target]);
    }
}

static void contacts_tick(lv_timer_t * timer)
{
    LV_UNUSED(timer);
    chat_speaker_count();          /* the timeout sweep runs here, and its
                                      removal is a version bump we notice */
    unsigned cv = contacts_version();
    unsigned sv = chat_speaker_version();
    if(cv != g_ui.c_ver || sv != g_ui.s_ver) rebuild();
}

/* ---------- build and destroy ---------- */

lv_obj_t * contacts_ui_create(lv_obj_t * parent, lv_group_t * group)
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
    lv_obj_set_style_pad_row(root, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(root, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(root, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(root, LV_DIR_NONE);
    lv_obj_set_scrollable(root, false);
    g_ui.root = root;

    grit_backdrop(root, 800, 452);

    /* the strip: the same little square and title the other panels wear */
    grit_square(root, 8, 9, 8, PDA_MSG_ACCENT);
    g_ui.title = lv_label_create(root);
    lv_label_set_text(g_ui.title, tr(TR_TAB_CONTACTS));
    lv_obj_set_pos(g_ui.title, 24, 5);
    lv_obj_set_style_text_font(g_ui.title, &pda_font_mono_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(g_ui.title, lv_color_hex(PDA_MSG_ACCENT), LV_PART_MAIN);
    grit_rule(root, 6, STRIP_H + 2, 788, false, PDA_RULE);

    lv_obj_t * frame = grit_frame(root, FRAME_X, FRAME_Y, FRAME_W, FRAME_H, PDA_FRAME);
    if(frame) {
        lv_obj_set_style_bg_color(frame, lv_color_hex(PDA_PAGE_BG), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(frame, LV_OPA_90, LV_PART_MAIN);
    }
    lv_obj_t * panel = frame ? frame : root;

    lv_obj_t * list = lv_obj_create(panel);
    lv_obj_set_pos(list, LIST_PAD, LIST_PAD);
    lv_obj_set_size(list, FRAME_W - LIST_PAD * 2, FRAME_H - LIST_PAD * 2);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(list, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(list, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_row(list, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_column(list, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);
    lv_obj_set_scrollable(list, true);
    lv_obj_set_style_bg_color(list, lv_color_hex(PDA_SCROLL), LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_opa(list, LV_OPA_COVER, LV_PART_SCROLLBAR);
    lv_obj_set_style_width(list, 4, LV_PART_SCROLLBAR);
    lv_obj_set_style_radius(list, 0, LV_PART_SCROLLBAR);
    g_ui.list = list;
    if(group) lv_group_add_obj(group, list);

    if(frame) grit_corners(frame, FRAME_W, FRAME_H, PDA_FRAME);

    build_rows();
    g_ui.c_ver = contacts_version();
    g_ui.s_ver = chat_speaker_version();

    g_ui.tick = lv_timer_create(contacts_tick, 500, NULL);
    return root;
}

void contacts_ui_destroy(void)
{
    cancel_flashes();
    if(g_ui.tick) {
        lv_timer_delete(g_ui.tick);
        g_ui.tick = NULL;
    }
    if(g_ui.root) {
        /* animations first: one that outlives its object is a crash */
        lv_kill_tree(g_ui.root);
        /* then the tree itself - which also takes its widgets out of the
           keypad group (the shell does not delete this pointer again) */
        lv_obj_delete(g_ui.root);
    }
    memset(&g_ui, 0, sizeof(g_ui));
}
