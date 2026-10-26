#include "ui/wallet_ui.h"
#include "ui/pda_shell.h"
#include "wallet/wallet.h"
#include "wallet/qr.h"
#include "ui/palette.h"
#include "ui/i18n.h"
#include "ui/grit.h"
#include "ui/lvutil.h"
#include "assets/fonts/pda_fonts.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The Wallet tab.
 *
 * Five screens: the balance (which is also where "not set up" and the error
 * live), REQUEST - an amount, a memo and a QR code of a bolt11 the backend
 * just made - SEND, where an invoice from a paste or from PDA_INVOICE is
 * offered for payment, CONFIRM, where its amount and fee are shown before
 * the PAY button will move anything, and HISTORY, the rows the backend has
 * kept. SCAN stays dimmed with its reason: the camera is a screen this
 * build does not have, and nothing here pretends to work.
 *
 *   MAIN                SEND                CONFIRM PAY
 *   +--------------+  +-----------------+  +------------------+
 *   | WALLET  MOCK |  | INVOICE         |  | INVOICE lnbc...  |
 *   |   1 234 567  |  | [............ ] |  | MEMO    coffee   |
 *   |     sats     |  | MEMO [coffee  ] |  | AMOUNT  500 sats |
 *   | [REQ][SEND]  |  | [CHECK] [SCAN ] |  | fee     1 sats   |
 *   | [HIST][SCAN] |  | (the reason)    |  | [CANCEL]  [PAY]  |
 *   +--------------+  +-----------------+  +------------------+
 *
 * Everything these screens show comes from wallet.h: check, fee, pay and
 * history are the seam, and the mock behind them moves numbers on a timer
 * and writes them to disk. A real backend replaces the mock and the screens
 * do not change. */

#define BACK_W        800
#define BACK_H        452
#define TITLE_RULE_Y  30

/* the balance screen: the rule and the row of buttons along the bottom */
#define MAIN_RULE_Y   306
#define MAIN_BTN_Y    340
#define MAIN_BTN_H    32
#define MAIN_BTN_W    156
#define MAIN_BTN_X0   64
#define MAIN_BTN_GAP  16
#define MAIN_WHY_Y    (MAIN_BTN_Y + MAIN_BTN_H + 4)

/* the REQUEST screen */
#define AM_X          24
#define AM_W          140
#define AM_LBL_Y      42
#define AM_VAL_Y      58
#define STEP_Y        54
#define STEP_X0       180
#define STEP_W        64
#define STEP_H        26
#define STEP_GAP      8
#define MEM_LBL_Y     90
#define MEM_X         24
#define MEM_Y         106
#define MEM_W         440
#define MEM_H         28
#define REQ_RULE_Y    142
#define QR_F_X        32
#define QR_F_Y        150
#define QR_F_W        296
#define QR_F_H        292
#define BACK_BTN_X    696
#define BACK_BTN_Y    4
#define BACK_BTN_W    68
#define BACK_BTN_H    22
#define COL_X         348
#define COL_W         432
#define COL_INV_Y     156
#define COL_EXP_Y     178
#define COL_STA_Y     202
#define NEWRQ_Y       240
#define NEWRQ_W       180
#define NEWRQ_H       30
#define COL_RULE_Y    310
#define COL_MOCK_Y    330
#define COL_HINT_Y    350

#define AMOUNT_MIN    1
#define AMOUNT_MAX    100000000LL      /* one bitcoin, in sats */

/* the SEND screen */
#define SEND_LBL_Y    44
#define SEND_INV_Y    58
#define SEND_INV_H    64
#define SEND_MEM_Y    132
#define SEND_MEMO_Y   146
#define SEND_MEMO_H   28
#define SEND_FLD_X    24
#define SEND_FLD_W    752
#define SEND_RULE_Y   186
#define SEND_BTN_Y    198
#define SEND_BTN_H    32
#define SEND_CHECK_X  24
#define SEND_CHECK_W  160
#define SEND_SCAN_X   208
#define SEND_SCAN_W   160
#define SEND_WHY_Y    (SEND_BTN_Y + SEND_BTN_H + 4)
#define SEND_ST_Y     256
#define SEND_HINT_Y   298

/* the CONFIRM screen (and the face that says the payment went out) */
#define CONF_LBL_Y    44
#define CONF_INV_Y    58
#define CONF_MEM_Y    88
#define CONF_MEMO_Y   102
#define CONF_RULE_Y   136
#define CONF_K_X      24
#define CONF_K_W      180
#define CONF_V_X      220
#define CONF_V_W      556
#define CONF_AMT_Y    156
#define CONF_FEE_Y    196
#define CONF_RULE2_Y  238
#define CONF_ST_Y     254
#define CONF_BTN_Y    292
#define CONF_BTN_H    34
#define CONF_CANC_X   200
#define CONF_PAY_X    420
#define CONF_BTN_W    180
#define CONF_MOCK_Y   346
#define SENT_TITLE_Y  150
#define SENT_AMT_Y    196
#define SENT_NOTE_Y   224
#define SENT_BAL_Y    248
#define SENT_BTN_Y    292

/* the HISTORY screen */
#define HIST_LIST_X   16
#define HIST_LIST_Y   44
#define HIST_LIST_W   768
#define HIST_LIST_H   364
#define HIST_ROW_H    24
#define HIST_MAX      64          /* the seam holds no more than this */
#define H_DIR_X       6
#define H_DIR_W       34
#define H_AMT_X       46
#define H_AMT_W       110
#define H_MEMO_X      168
#define H_MEMO_W      344
#define H_DATE_X      524
#define H_DATE_W      84
#define H_STAT_X      624
#define H_STAT_W      122

enum {
    SCREEN_MAIN = 0,
    SCREEN_REQUEST = 1,
    SCREEN_SEND = 2,
    SCREEN_CONFIRM = 3,
    SCREEN_HISTORY = 4
};

typedef struct {
    lv_obj_t * root;
    lv_obj_t * content;       /* the screen that is up, rebuilt on navigation */
    lv_timer_t * tick;        /* one timer for the whole panel */
    lv_group_t * group;       /* the keypad group: Pi buttons, no touch */

    /* the REQUEST screen - all NULL while another screen is up */
    lv_obj_t * amount_lbl;
    lv_obj_t * memo;
    lv_obj_t * frame;
    lv_obj_t * qr;            /* the canvas the QR pixels live in */
    lv_obj_t * qr_err;
    lv_obj_t * paid_box;
    lv_obj_t * paid_title;
    lv_obj_t * paid_amt;
    lv_obj_t * paid_note;
    lv_obj_t * paid_bal;
    lv_obj_t * invoice_lbl;
    lv_obj_t * expires_lbl;
    lv_obj_t * status_lbl;
    lv_obj_t * newrq;
    lv_obj_t * hint;
    uint32_t * qr_px;         /* the canvas buffer - ours to free */
    int inv_state;            /* wallet_invoice_state_t last applied, -1 = none */

    /* the SEND screen */
    lv_obj_t * send_inv;      /* the field the bolt11 sits in */
    lv_obj_t * send_memo;
    lv_obj_t * send_status;   /* where a refused invoice is explained */

    /* the CONFIRM screen */
    lv_obj_t * conf_status;

    /* the HISTORY screen */
    lv_obj_t * hist_list;
} wallet_ui_t;

static wallet_ui_t g_wui;

/* these outlive the panel: a theme change rebuilds the screen you were on,
   with the amount and the memo you had typed */
static int s_screen = SCREEN_MAIN;
static int64_t s_amount = 500;
static char s_memo[48];
static char s_last_err[192];

/* the same, for the payment: the invoice survives a theme change so the
   confirm screen comes back with the same numbers on it */
static char s_invoice[640];
static bool s_invoice_loaded;  /* PDA_INVOICE has been looked for already */
static char s_pay_memo[48];
static char s_send_err[192];
static int64_t s_confirm_sats;
static int64_t s_confirm_fee;
static bool s_sent;            /* the payment went through: the result face */

/* the newest history row the HISTORY screen has drawn, so a payment that
   lands while it is open can be noticed without rebuilding every tick */
static wallet_tx_t s_hist_top;
static bool s_hist_top_valid;

/* ---------- small helpers ---------- */

static lv_obj_t * line(lv_obj_t * parent, const char * txt, int x, int y, int w,
                       const lv_font_t * font, uint32_t col)
{
    lv_obj_t * l = lv_label_create(parent);
    lv_label_set_text(l, txt);
    lv_obj_set_pos(l, x, y);
    lv_obj_set_width(l, w);
    lv_obj_set_style_text_font(l, font, LV_PART_MAIN);
    lv_obj_set_style_text_color(l, lv_color_hex(col), LV_PART_MAIN);
    lv_obj_set_style_pad_all(l, 0, LV_PART_MAIN);
    return l;
}

/* one line, cut short with dots rather than allowed to grow */
static lv_obj_t * line_dot(lv_obj_t * parent, const char * txt, int x, int y, int w,
                           const lv_font_t * font, uint32_t col)
{
    lv_obj_t * l = line(parent, txt, x, y, w, font, col);
    /* LVGL keeps the object's size and writes the dots where the text stops
       - so the height is what tells it "one line, no more". Without it a
       long bolt11 grows into the row under it. */
    lv_obj_set_height(l, lv_font_get_line_height(font));
    lv_label_set_long_mode(l, LV_LABEL_LONG_DOT);
    return l;
}

/* one centered line: it never grows past its width, it gets cut short */
static void center_line(lv_obj_t * l)
{
    lv_label_set_long_mode(l, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
}

/* a centered paragraph */
static void center_block(lv_obj_t * l)
{
    lv_label_set_long_mode(l, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
}

/* 1234567 -> "1 234 567" (the seven-segment font has a space) */
static void format_sats(int64_t sats, char * buf, size_t n)
{
    if(!buf || !n) return;
    buf[0] = 0;
    if(sats < 0) sats = 0;

    char digits[24];
    int len = snprintf(digits, sizeof(digits), "%lld", (long long)sats);
    if(len < 0) return;

    size_t out = 0;
    for(int i = 0; i < len; i++) {
        if(i > 0 && (len - i) % 3 == 0) {
            if(out + 3 >= n) break;      /* room for the space, a digit, the end */
            buf[out++] = ' ';
        }
        if(out + 2 >= n) break;          /* room for the digit and the end */
        buf[out++] = digits[i];
    }
    buf[out] = 0;
}

/* 1234567 -> "0.01234567 BTC" */
static void format_btc(int64_t sats, char * buf, size_t n)
{
    if(!buf || !n) return;
    if(sats < 0) sats = 0;
    long long whole = (long long)(sats / 100000000LL);
    long long frac = (long long)(sats % 100000000LL);
    snprintf(buf, n, "%lld.%08lld BTC", whole, frac);
}

/* the strip every wallet screen wears: two squares, the title, the rule */
static void build_title(lv_obj_t * p, const char * banner)
{
    grit_square(p, 22, 9, 8, PDA_MSG_ACCENT);
    grit_square(p, 770, 9, 8, PDA_MSG_ACCENT);

    lv_obj_t * t = line(p, tr(TR_TAB_WALLET), 280, 5, 240,
                        &pda_font_mono_16, PDA_MSG_TEXT);
    center_line(t);

    if(banner && *banner) {
        lv_obj_t * b = line(p, banner, 400, 8, 330,
                            &pda_font_mono_10, PDA_MSG_ACCENT);
        lv_label_set_long_mode(b, LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_align(b, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    }

    grit_rule(p, 6, TITLE_RULE_Y, 788, false, PDA_RULE);
}

/* a button that is there to be seen, not to be pressed: outside the keypad
 * group, not clickable, and saying why underneath */
static void dim_button(lv_obj_t * p, const char * txt, int x, int y, int w)
{
    lv_obj_t * b = lv_button_create(p);
    lv_obj_set_pos(b, x, y);
    lv_obj_set_size(b, w, MAIN_BTN_H);
    lv_obj_set_style_bg_color(b, lv_color_hex(PDA_SUB_FILL), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(b, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(b, lv_color_hex(PDA_SUB_EDGE), LV_PART_MAIN);
    lv_obj_set_style_radius(b, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_all(b, 0, LV_PART_MAIN);
    lv_no_transition(b);

    lv_obj_t * l = lv_label_create(b);
    lv_label_set_text(l, txt);
    lv_obj_set_pos(l, 0, (MAIN_BTN_H - 16) / 2);
    lv_obj_set_width(l, w);
    lv_label_set_long_mode(l, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(l, &pda_font_mono_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(l, lv_color_hex(PDA_MSG_SYS), LV_PART_MAIN);
    lv_obj_set_style_pad_all(l, 0, LV_PART_MAIN);

    /* No LV_STATE_DISABLED here on purpose. The default theme styles that
       state with its own grey plate and a recoloured label, and it wins the
       cascade for a state the styles above were not written for - the plate
       came out light grey and the label with it, on this screen unreadable.
       Not clickable (and never in the keypad group) is what stops the press. */
    lv_obj_set_clickable(b, false);
}

/* the reason a dimmed button is dimmed, in its own column under it */
static void why_line(lv_obj_t * p, const char * txt, int x, int y, int w)
{
    lv_obj_t * l = line(p, txt, x, y, w, &pda_font_mono_10, PDA_MSG_TIME);
    center_line(l);
}

/* a button that works: in the keypad group, accent edge and accent label */
static lv_obj_t * live_button(lv_obj_t * p, const char * txt, int x, int y, int w, int h,
                        lv_event_cb_t cb, void * user, lv_group_t * group)
{
    lv_obj_t * b = lv_button_create(p);
    lv_obj_set_pos(b, x, y);
    lv_obj_set_size(b, w, h);
    lv_obj_set_style_bg_color(b, lv_color_hex(PDA_SUB_FILL), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(b, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(b, lv_color_hex(PDA_MSG_ACCENT), LV_PART_MAIN);
    lv_obj_set_style_radius(b, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_all(b, 0, LV_PART_MAIN);
    lv_no_transition(b);

    lv_obj_t * l = lv_label_create(b);
    lv_label_set_text(l, txt);
    lv_obj_set_pos(l, 0, (h - 16) / 2);
    lv_obj_set_width(l, w);
    lv_label_set_long_mode(l, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(l, &pda_font_mono_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(l, lv_color_hex(PDA_MSG_ACCENT), LV_PART_MAIN);
    lv_obj_set_style_pad_all(l, 0, LV_PART_MAIN);

    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, user);
    if(group) lv_group_add_obj(group, b);
    return b;
}

/* a text field: the plate the REQUEST memo wears, for every field here */
static lv_obj_t * field(lv_obj_t * p, int x, int y, int w, int h,
                        const lv_font_t * font, bool one_line,
                        const char * txt, int max_len)
{
    lv_obj_t * ta = lv_textarea_create(p);
    if(one_line) lv_textarea_set_one_line(ta, true);
    lv_textarea_set_max_length(ta, (uint32_t)max_len);
    lv_textarea_set_text(ta, txt ? txt : "");
    lv_obj_set_pos(ta, x, y);
    lv_obj_set_size(ta, w, h);
    /* the field must not shrink to the size of its text */
    lv_obj_set_style_min_width(ta, w, LV_PART_MAIN);
    lv_obj_set_style_bg_color(ta, lv_color_hex(PDA_CHAT_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ta, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(ta, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(ta, lv_color_hex(PDA_CHAT_EDGE), LV_PART_MAIN);
    lv_obj_set_style_text_font(ta, font, LV_PART_MAIN);
    lv_obj_set_style_text_color(ta, lv_color_hex(PDA_MSG_TEXT), LV_PART_MAIN);
    lv_obj_set_style_text_color(ta, lv_color_hex(PDA_MSG_SYS), LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_pad_all(ta, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_left(ta, 6, LV_PART_MAIN);
    lv_caret_init(ta);
    if(g_wui.group) lv_group_add_obj(g_wui.group, ta);
    return ta;
}

/* ---------- forward declarations ---------- */

static void on_open_request(lv_event_t * e);
static void on_open_send(lv_event_t * e);
static void on_open_history(lv_event_t * e);
static void on_back(lv_event_t * e);
static void on_new_request(lv_event_t * e);
static void on_step(lv_event_t * e);
static void on_memo(lv_event_t * e);
static void on_pay_memo(lv_event_t * e);
static void on_check(lv_event_t * e);
static void on_cancel(lv_event_t * e);
static void on_pay(lv_event_t * e);
static void on_sent_done(lv_event_t * e);
static void build_current(void);
static void content_clear(void);
static void regenerate(void);
static void refresh_request(void);
static void refresh_history(void);
static void rebuild(void);

/* every screen change here is the same two steps: let the tree go, build
   the one that was asked for */
static void rebuild(void)
{
    content_clear();
    build_current();
}

/* ---------- the three states ---------- */

/* the paste door of the NWC backend: present on the not-set-up screen only
   while there is no connection (section 7) - the replace semantics under it
   are built, but v1 reaches the door only from here */
static bool s_conn_armed = false;      /* [PASTE] first press arms, like [NEW] */
static bool s_conn_logged = false;     /* boxes printed once, on the first tick */
static char s_conn_err[192];           /* the last refusal, on the why-line */
static lv_obj_t * s_conn_ta = NULL;
static lv_obj_t * s_conn_btn_lbl = NULL;
static int s_seen_state = -1;          /* the tick's state-change rebuild */

static bool nwc_needing_connection(void)
{
    return wallet_state() == WALLET_UNSET &&
           strcmp(wallet_backend_name(), "nwc") == 0;
}

static void do_connect(void);
static void on_conn_click(lv_event_t * e);

/* The paste field's box, printed from the tick (like the Messages compose
   row in slice C): at panel-create time the shell has not positioned the
   tab panel yet, so a box measured there lands off by the tab row - the
   suite clicks what this line says, so it has to be the true box. */
static void log_paste_boxes(void)
{
    if(s_conn_logged || !s_conn_ta) return;
    s_conn_logged = true;

    lv_obj_update_layout(s_conn_ta);
    lv_area_t a;
    lv_obj_get_coords(s_conn_ta, &a);
    fprintf(stderr, "[wallet] paste field: x=%d y=%d w=%d h=%d placeholder=%s\n",
            a.x1, a.y1, lv_obj_get_width(s_conn_ta),
            lv_obj_get_height(s_conn_ta),
            lv_textarea_get_placeholder_text(s_conn_ta));

    if(s_conn_btn_lbl) {
        lv_obj_t * btn = lv_obj_get_parent(s_conn_btn_lbl);
        lv_obj_update_layout(btn);
        lv_obj_get_coords(btn, &a);
        fprintf(stderr, "[wallet] paste button: x=%d y=%d w=%d h=%d\n",
                a.x1, a.y1, lv_obj_get_width(btn), lv_obj_get_height(btn));
    }
}

/* no backend at all - this is what the device honestly is today. With
   PDA_WALLET_BACKEND=nwc and no connection saved, the copy changes with
   it: the "nothing installed" body and the "later build" why-line would
   both be false now, so the backend's own reason takes the body slot, the
   CREATE/RESTORE buttons (NWC has no seed) are hidden, and the paste field
   takes their row. Everything else on this screen is unchanged. */
static void build_unset(lv_obj_t * p)
{
    build_title(p, NULL);
    s_conn_armed = false;
    s_conn_logged = false;
    s_conn_ta = NULL;
    s_conn_btn_lbl = NULL;

    bool nwc = nwc_needing_connection();

    lv_obj_t * h = line(p, tr(TR_WALLET_NOT_SET_UP), 0, 140, BACK_W,
                        &pda_font_mono_16, PDA_MSG_ACCENT);
    center_line(h);

    const char * body = tr(TR_WALLET_NO_BACKEND);
    if(nwc && wallet_error()[0]) body = wallet_error();   /* backend English */
    lv_obj_t * b = line(p, body, 120, 182, 560,
                        &pda_font_mono_14, PDA_TEXT);
    center_block(b);

    if(nwc) {
        /* the field the connection URI is pasted into, where CREATE and
           RESTORE used to stand - their dimmed row is this backend's row */
        s_conn_ta = field(p, 170, 262, 440, 32, &pda_font_mono_10, true,
                          "", 1000);
        lv_textarea_set_placeholder_text(s_conn_ta, tr(TR_WALLET_CONN_HINT));

        lv_obj_t * pb = live_button(p, tr(TR_KEY_PASTE), 620, 262, 120,
                                    MAIN_BTN_H, on_conn_click, NULL,
                                    g_wui.group);
        /* the first press arms it (label flips to SURE?), the second one
           acts - the User page's [NEW] pattern, so a fat finger does not
           swap a wallet connection (section 2's confirm lean) */
        s_conn_btn_lbl = lv_obj_get_child(pb, 0);

        lv_obj_t * why = line(p, s_conn_err, 0, 312, BACK_W,
                              &pda_font_mono_10,
                              s_conn_err[0] ? PDA_CARRIER_DOWN : PDA_MSG_TIME);
        center_line(why);
    }
    else {
        dim_button(p, tr(TR_WALLET_CREATE), 236, 262, 156);
        dim_button(p, tr(TR_WALLET_RESTORE), 408, 262, 156);

        lv_obj_t * why = line(p, tr(TR_WALLET_COMING), 0, 312, BACK_W,
                              &pda_font_mono_10, PDA_MSG_TIME);
        center_line(why);
    }
}

/* The paste field's [PASTE]: validate and save through the seam's one URI
   door (wallet_connect_uri), then show whatever the backend answered - a
   saved connection re-ran wallet.c's init, so the rebuild shows the real
   state: READY behind a fetch, or the honest unreachable reason. A refusal
   wrote nothing and says why on the why-line below. */
static void do_connect(void)
{
    const char * txt = s_conn_ta ? lv_textarea_get_text(s_conn_ta) : "";
    char err[192];
    s_conn_err[0] = 0;
    if(!wallet_connect_uri(txt, err, sizeof(err)))
        snprintf(s_conn_err, sizeof(s_conn_err), "%s",
                 err[0] ? err : tr(TR_FAILED));
    s_conn_armed = false;
    rebuild();
}

static void on_conn_click(lv_event_t * e)
{
    (void)e;
    if(!s_conn_armed) {
        s_conn_armed = true;
        if(s_conn_btn_lbl) lv_label_set_text(s_conn_btn_lbl, tr(TR_KEY_SURE));
        return;
    }
    do_connect();
}

/* a backend is up: the balance screen (today the mock or nwc get here).
   The MOCK wording is the preview backend's and nothing else's - it is gated
   on wallet_is_mock() so a real backend behind the same seam never wears it. */
static void build_ready(lv_obj_t * p)
{
    build_title(p, wallet_is_mock() ? tr(TR_WALLET_MOCK_BANNER) : NULL);

    char sats[32], btc[32];
    int64_t bal = wallet_balance_sats();
    format_sats(bal, sats, sizeof(sats));
    format_btc(bal, btc, sizeof(btc));

    lv_obj_t * n = line(p, sats, 0, 64, BACK_W, &pda_font_seg7_28, PDA_TEXT);
    center_line(n);

    lv_obj_t * unit = line(p, tr(TR_WALLET_SATS), 0, 104, BACK_W,
                           &pda_font_mono_14, PDA_MSG_TIME);
    center_line(unit);

    lv_obj_t * b = line(p, btc, 0, 130, BACK_W, &pda_font_mono_10, PDA_MSG_TIME);
    center_line(b);

    grit_rule(p, 6, 162, 788, false, PDA_RULE);

    lv_obj_t * k1 = line(p, tr(TR_WALLET_STATE), 250, 184, 70,
                         &pda_font_mono_10, PDA_MSG_SYS);
    lv_obj_set_style_text_align(k1, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    line(p, wallet_is_mock() ? tr(TR_WALLET_MOCK_STATE)
                             : wallet_backend_name(),
         340, 184, 300, &pda_font_mono_10, PDA_MSG_ACCENT);

    /* "not connected" is a statement about the preview only - a real
       backend's link state is its own to report, and there is no call for
       it in the seam yet, so the row is simply not drawn */
    if(wallet_is_mock()) {
        lv_obj_t * k2 = line(p, tr(TR_WALLET_NET), 250, 208, 70,
                             &pda_font_mono_10, PDA_MSG_SYS);
        lv_obj_set_style_text_align(k2, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
        line(p, tr(TR_WALLET_NET_OFF), 340, 208, 300,
             &pda_font_mono_10, PDA_MSG_TEXT);
    }

    /* the actions. Three of them have a screen behind them; SCAN needs the
       camera, which this build does not have, so it says so instead of
       looking broken */
    grit_rule(p, 6, MAIN_RULE_Y, 788, false, PDA_RULE);

    live_button(p, tr(TR_WALLET_REQUEST), MAIN_BTN_X0, MAIN_BTN_Y,
                MAIN_BTN_W, MAIN_BTN_H, on_open_request, NULL, g_wui.group);

    int x1 = MAIN_BTN_X0 + (MAIN_BTN_W + MAIN_BTN_GAP);
    int x2 = x1 + (MAIN_BTN_W + MAIN_BTN_GAP);
    int x3 = x2 + (MAIN_BTN_W + MAIN_BTN_GAP);

    live_button(p, tr(TR_SEND), x1, MAIN_BTN_Y,
                MAIN_BTN_W, MAIN_BTN_H, on_open_send, NULL, g_wui.group);

    live_button(p, tr(TR_WALLET_HISTORY), x2, MAIN_BTN_Y,
                MAIN_BTN_W, MAIN_BTN_H, on_open_history, NULL, g_wui.group);

    dim_button(p, tr(TR_WALLET_SCAN), x3, MAIN_BTN_Y, MAIN_BTN_W);
    why_line(p, tr(TR_WALLET_WHY_CAMERA), x3, MAIN_WHY_Y, MAIN_BTN_W);
}

/* PDA_WALLET_BACKEND asked for something this build does not have */
static void build_error(lv_obj_t * p)
{
    build_title(p, NULL);

    lv_obj_t * h = line(p, tr(TR_WALLET_ERROR), 0, 140, BACK_W,
                        &pda_font_mono_16, PDA_MSG_ACCENT);
    center_line(h);

    lv_obj_t * b = line(p, wallet_error(), 100, 182, 600,
                        &pda_font_mono_14, PDA_TEXT);
    center_block(b);
}

/* ---------- REQUEST ---------- */

static void update_amount_label(void)
{
    if(!g_wui.amount_lbl) return;
    char num[32], buf[48];
    format_sats(s_amount, num, sizeof(num));
    snprintf(buf, sizeof(buf), "%s %s", num, tr(TR_WALLET_SATS));
    lv_label_set_text(g_wui.amount_lbl, buf);
}

/* ask the backend for an invoice of what is on the screen right now */
static void make_invoice_now(void)
{
    s_last_err[0] = 0;
    char bolt11[640];
    if(!wallet_make_invoice(s_amount, s_memo, bolt11, sizeof(bolt11),
                            s_last_err, sizeof(s_last_err)) && !s_last_err[0])
        snprintf(s_last_err, sizeof(s_last_err), "%s", tr(TR_WALLET_MAKE_FAILED));
}

/* draw the invoice into the canvas (or say why there is nothing to draw) */
static void refresh_qr(void)
{
    if(!g_wui.qr) return;

    wallet_invoice_t inv;
    if(!wallet_invoice(&inv) || !inv.valid) {
        lv_obj_set_hidden(g_wui.qr, true);
        if(g_wui.qr_err) {
            lv_label_set_text(g_wui.qr_err,
                              s_last_err[0] ? s_last_err : tr(TR_WALLET_QR_FAIL));
            lv_obj_set_hidden(g_wui.qr_err, false);
        }
        return;
    }

    qr_image_t img;
    if(!qr_render(inv.bolt11, &img)) {
        lv_obj_set_hidden(g_wui.qr, true);
        if(g_wui.qr_err) {
            lv_label_set_text(g_wui.qr_err, tr(TR_WALLET_QR_FAIL));
            lv_obj_set_hidden(g_wui.qr_err, false);
        }
        return;
    }
    if(g_wui.qr_err) lv_obj_set_hidden(g_wui.qr_err, true);

    /* point the canvas at the new pixels first, then let the old ones go */
    lv_canvas_set_buffer(g_wui.qr, img.px, img.side, img.side,
                         LV_COLOR_FORMAT_ARGB8888);
    if(g_wui.qr_px) free(g_wui.qr_px);
    g_wui.qr_px = img.px;
    lv_obj_set_size(g_wui.qr, img.side, img.side);
    lv_obj_set_pos(g_wui.qr, (QR_F_W - img.side) / 2, (QR_F_H - img.side) / 2);
    lv_obj_set_hidden(g_wui.qr, false);
}

/* the waiting / paid / expired faces of the screen */
static void apply_request_state(const wallet_invoice_t * inv)
{
    bool paid = inv->state == WALLET_INVOICE_PAID;
    bool waiting = inv->state == WALLET_INVOICE_WAITING;

    if(g_wui.qr) lv_obj_set_hidden(g_wui.qr, paid);
    if(g_wui.paid_box) lv_obj_set_hidden(g_wui.paid_box, !paid);
    if(g_wui.newrq) lv_obj_set_hidden(g_wui.newrq, waiting);
    if(g_wui.hint) lv_obj_set_hidden(g_wui.hint, !waiting);
    if(g_wui.expires_lbl) lv_obj_set_hidden(g_wui.expires_lbl, paid);

    if(g_wui.status_lbl) {
        uint32_t col = paid ? PDA_MSG_ACCENT
                            : (waiting ? PDA_MSG_TEXT : PDA_MSG_TIME);
        lv_obj_set_style_text_color(g_wui.status_lbl, lv_color_hex(col), LV_PART_MAIN);
        lv_label_set_text(g_wui.status_lbl,
                          paid ? tr(TR_WALLET_PAID)
                              : (waiting ? tr(TR_WALLET_WAITING)
                                         : tr(TR_WALLET_EXPIRED)));
    }

    if(paid && g_wui.paid_title && g_wui.paid_amt &&
       g_wui.paid_note && g_wui.paid_bal) {
        char num[32], buf[64];
        lv_label_set_text(g_wui.paid_title, tr(TR_WALLET_PAID));
        format_sats(inv->sats, num, sizeof(num));
        snprintf(buf, sizeof(buf), "+ %s %s", num, tr(TR_WALLET_SATS));
        lv_label_set_text(g_wui.paid_amt, buf);
        lv_label_set_text(g_wui.paid_note,
                          wallet_is_mock() ? tr(TR_WALLET_PAID_NOTE) : "");
        format_sats(wallet_balance_sats(), num, sizeof(num));
        snprintf(buf, sizeof(buf), "%s %s %s", tr(TR_WALLET_BALANCE),
                 num, tr(TR_WALLET_SATS));
        lv_label_set_text(g_wui.paid_bal, buf);
    }
}

/* the countdown, and any change of state since the last look */
static void refresh_request(void)
{
    if(s_screen != SCREEN_REQUEST || !g_wui.content) return;

    wallet_invoice_t inv;
    if(!wallet_invoice(&inv) || !inv.valid) return;

    if(g_wui.expires_lbl && inv.state != WALLET_INVOICE_PAID) {
        long left = (long)(inv.expires_at - time(NULL));
        if(left < 0) left = 0;
        char buf[48];
        snprintf(buf, sizeof(buf), "%s %02ld:%02ld", tr(TR_WALLET_EXPIRES),
                 left / 60, left % 60);
        lv_label_set_text(g_wui.expires_lbl, buf);
    }

    if(g_wui.inv_state == (int)inv.state) return;
    g_wui.inv_state = (int)inv.state;
    apply_request_state(&inv);
}

/* what the bolt11 is, or that there is none */
static void update_invoice_label(const wallet_invoice_t * inv)
{
    if(!g_wui.invoice_lbl) return;
    if(inv && inv->valid) lv_label_set_text(g_wui.invoice_lbl, inv->bolt11);
    else lv_label_set_text(g_wui.invoice_lbl, "-");
}

/* with no invoice at all the screen must say so rather than look broken */
static void show_no_invoice(void)
{
    if(g_wui.status_lbl)
        lv_label_set_text(g_wui.status_lbl,
                          s_last_err[0] ? s_last_err : tr(TR_WALLET_QR_FAIL));
    if(g_wui.expires_lbl) lv_obj_set_hidden(g_wui.expires_lbl, true);
    if(g_wui.hint) lv_obj_set_hidden(g_wui.hint, true);
    if(g_wui.newrq) lv_obj_set_hidden(g_wui.newrq, false);
    if(g_wui.paid_box) lv_obj_set_hidden(g_wui.paid_box, true);
    if(g_wui.qr) lv_obj_set_hidden(g_wui.qr, true);
}

/* read whatever the backend is holding and put it on the screen */
static void show_invoice(void)
{
    wallet_invoice_t inv;
    bool ok = wallet_invoice(&inv) && inv.valid;

    update_invoice_label(ok ? &inv : NULL);
    refresh_qr();

    if(!ok) {
        show_no_invoice();
        return;
    }
    g_wui.inv_state = -1;
    refresh_request();
}

/* the mock settles on the wall clock, so the panel is polled for it - and
   it keeps its own time wherever in the tab we happen to be looking. nwc
   pumps its in-flight work and cadences from the same pass (wallet_poll),
   and this is also where a state change swaps the screen: the tick compares
   wallet_state() each pass and rebuilds only on a change, so the retry path
   reaching READY (or a transport failure demoting) rebuilds once instead of
   churning every 250 ms. */
static void wallet_tick(lv_timer_t * t)
{
    (void)t;
    wallet_poll();
    if(getenv("PDA_DEBUG_FIELD")) log_paste_boxes();

    int st = (int)wallet_state();
    if(st != s_seen_state) {
        s_seen_state = st;
        rebuild();
        return;
    }

    if(s_screen == SCREEN_REQUEST) refresh_request();
    else if(s_screen == SCREEN_HISTORY) refresh_history();
}

/* a new invoice for whatever is on the screen now */
static void regenerate(void)
{
    if(s_screen != SCREEN_REQUEST) return;
    make_invoice_now();
    show_invoice();
}

static void on_open_request(lv_event_t * e)
{
    (void)e;
    if(wallet_state() != WALLET_READY) return;
    make_invoice_now();
    s_screen = SCREEN_REQUEST;
    content_clear();
    build_current();
}

static void on_back(lv_event_t * e)
{
    (void)e;
    s_screen = SCREEN_MAIN;
    content_clear();
    build_current();
}

static void on_new_request(lv_event_t * e)
{
    (void)e;
    regenerate();
}

static void on_step(lv_event_t * e)
{
    int64_t delta = (int64_t)(lv_intptr_t)lv_event_get_user_data(e);
    int64_t v = s_amount + delta;
    if(v < AMOUNT_MIN) v = AMOUNT_MIN;
    if(v > AMOUNT_MAX) v = AMOUNT_MAX;
    if(v == s_amount) return;
    s_amount = v;
    update_amount_label();
    regenerate();
}

static void on_memo(lv_event_t * e)
{
    lv_obj_t * ta = lv_event_get_current_target(e);
    const char * txt = lv_textarea_get_text(ta);
    if(!txt) txt = "";
    if(strcmp(txt, s_memo) == 0) return;
    snprintf(s_memo, sizeof(s_memo), "%s", txt);
    regenerate();               /* a different memo, a different invoice */
}

/* ---------- SEND, CONFIRM, HISTORY ---------- */

/* The first time SEND is opened the simulator is handed an invoice to pay,
 * because there is no camera to scan one with. After that the field is the
 * user's own: a payment that went through clears it, so the same invoice
 * cannot be offered twice. */
static void load_env_invoice(void)
{
    if(s_invoice_loaded) return;
    s_invoice_loaded = true;
    const char * v = getenv("PDA_INVOICE");
    if(v && *v) snprintf(s_invoice, sizeof(s_invoice), "%s", v);
}

/* what the field holds, taken straight off the widget so nothing typed
 * into it can be missed */
static void read_send_invoice(void)
{
    if(!g_wui.send_inv) return;
    const char * txt = lv_textarea_get_text(g_wui.send_inv);
    if(!txt) return;
    snprintf(s_invoice, sizeof(s_invoice), "%s", txt);
}

static void set_send_status(const char * txt, bool good)
{
    if(!g_wui.send_status) return;
    lv_label_set_text(g_wui.send_status, txt ? txt : "");
    lv_obj_set_style_text_color(g_wui.send_status,
                                lv_color_hex(good ? PDA_MSG_ACCENT
                                                  : PDA_CARRIER_DOWN),
                                LV_PART_MAIN);
}

static void on_open_send(lv_event_t * e)
{
    (void)e;
    if(wallet_state() != WALLET_READY) return;
    load_env_invoice();
    s_send_err[0] = 0;
    s_sent = false;
    s_screen = SCREEN_SEND;
    rebuild();
}

static void on_open_history(lv_event_t * e)
{
    (void)e;
    if(wallet_state() != WALLET_READY) return;
    s_screen = SCREEN_HISTORY;
    rebuild();
}

static void on_pay_memo(lv_event_t * e)
{
    lv_obj_t * ta = lv_event_get_current_target(e);
    const char * txt = lv_textarea_get_text(ta);
    if(!txt) txt = "";
    if(strcmp(txt, s_pay_memo) == 0) return;
    snprintf(s_pay_memo, sizeof(s_pay_memo), "%s", txt);
}

/* ask the backend what the field holds: the amount the confirm screen is
 * about to show, or the reason there is nothing to show */
static void on_check(lv_event_t * e)
{
    (void)e;
    read_send_invoice();
    s_send_err[0] = 0;

    int64_t sats = 0;
    if(!wallet_check_invoice(s_invoice, &sats, s_send_err, sizeof(s_send_err))) {
        if(!s_send_err[0])
            snprintf(s_send_err, sizeof(s_send_err), "%s", tr(TR_WALLET_CANT_PAY));
        set_send_status(s_send_err, false);
        return;
    }

    s_confirm_sats = sats;
    s_confirm_fee = wallet_fee_sats(sats);
    s_screen = SCREEN_CONFIRM;
    rebuild();
}

static void on_cancel(lv_event_t * e)
{
    (void)e;
    s_send_err[0] = 0;
    s_screen = SCREEN_SEND;
    rebuild();
}

/* the moment the numbers move */
static void on_pay(lv_event_t * e)
{
    (void)e;
    s_send_err[0] = 0;

    int64_t paid = 0;
    if(!wallet_pay_invoice(s_invoice, s_pay_memo, &paid,
                           s_send_err, sizeof(s_send_err))) {
        if(!s_send_err[0])
            snprintf(s_send_err, sizeof(s_send_err), "%s", tr(TR_WALLET_PAY_FAILED));
        if(g_wui.conf_status) {
            lv_label_set_text(g_wui.conf_status, s_send_err);
            lv_obj_set_style_text_color(g_wui.conf_status,
                                        lv_color_hex(PDA_CARRIER_DOWN), LV_PART_MAIN);
        }
        return;
    }

    /* it has been paid: the field must not offer it a second time */
    s_invoice[0] = 0;
    s_sent = true;
    rebuild();
}

static void on_sent_done(lv_event_t * e)
{
    (void)e;
    s_sent = false;
    s_screen = SCREEN_MAIN;
    rebuild();
}

/* the strip every screen below the balance wears: the banner stays, and
   BACK takes the right-hand end of it where the balance screen has nothing
   (CONFIRM is the exception - CANCEL is the way back from there) */
static void build_sub_title(lv_obj_t * p, tr_id_t title, bool back)
{
    grit_square(p, 22, 9, 8, PDA_MSG_ACCENT);
    grit_square(p, 770, 9, 8, PDA_MSG_ACCENT);

    lv_obj_t * t = line(p, tr(title), 0, 5, BACK_W,
                        &pda_font_mono_16, PDA_MSG_TEXT);
    center_line(t);

    if(wallet_is_mock()) {          /* the preview's banner, and only its */
        lv_obj_t * b = line(p, tr(TR_WALLET_MOCK_BANNER), 430, 8, 240,
                            &pda_font_mono_10, PDA_MSG_ACCENT);
        lv_label_set_long_mode(b, LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_align(b, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    }

    if(back)
        live_button(p, tr(TR_WALLET_BACK), BACK_BTN_X, BACK_BTN_Y,
                    BACK_BTN_W, BACK_BTN_H, on_back, NULL, g_wui.group);

    grit_rule(p, 6, TITLE_RULE_Y, 788, false, PDA_RULE);
}

static void build_request(lv_obj_t * p)
{
    build_sub_title(p, TR_WALLET_REQUEST, true);

    /* --- the amount and the keypad that moves it --- */
    line(p, tr(TR_WALLET_AMOUNT), AM_X, AM_LBL_Y, AM_W,
         &pda_font_mono_10, PDA_MSG_SYS);
    g_wui.amount_lbl = line(p, "", AM_X, AM_VAL_Y, AM_W,
                            &pda_font_mono_16, PDA_MSG_TEXT);
    update_amount_label();

    static const char * step_txt[4] = { "-10", "-1", "+1", "+10" };
    static const int step_delta[4] = { -10, -1, 1, 10 };
    for(int i = 0; i < 4; i++) {
        live_button(p, step_txt[i], STEP_X0 + i * (STEP_W + STEP_GAP), STEP_Y,
                    STEP_W, STEP_H, on_step,
                    (void *)(lv_intptr_t)step_delta[i], g_wui.group);
    }

    /* --- what it is for --- */
    line(p, tr(TR_WALLET_MEMO), MEM_X, MEM_LBL_Y, MEM_W,
         &pda_font_mono_10, PDA_MSG_SYS);

    g_wui.memo = field(p, MEM_X, MEM_Y, MEM_W, MEM_H, &pda_font_mono_14, true,
                       s_memo, (int)(sizeof(s_memo) - 1));

    grit_rule(p, 6, REQ_RULE_Y, 788, false, PDA_RULE);

    /* --- the code itself, and the paid screen that replaces it --- */
    g_wui.frame = grit_frame(p, QR_F_X, QR_F_Y, QR_F_W, QR_F_H, PDA_FRAME);
    lv_obj_t * frame = g_wui.frame ? g_wui.frame : p;

    g_wui.qr = lv_canvas_create(frame);
    lv_obj_set_clickable(g_wui.qr, false);
    lv_obj_set_style_border_width(g_wui.qr, 0, LV_PART_MAIN);
    lv_obj_set_pos(g_wui.qr, 0, 0);
    lv_obj_set_hidden(g_wui.qr, true);          /* until refresh_qr fills it */

    g_wui.qr_err = line(frame, "", 8, QR_F_H / 2 - 8, QR_F_W - 16,
                        &pda_font_mono_14, PDA_MSG_TIME);
    center_line(g_wui.qr_err);
    lv_obj_set_hidden(g_wui.qr_err, true);

    lv_obj_t * paid = lv_obj_create(frame);
    lv_obj_set_pos(paid, 4, 4);
    lv_obj_set_size(paid, QR_F_W - 8, QR_F_H - 8);
    lv_obj_set_style_bg_opa(paid, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(paid, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(paid, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(paid, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(paid, LV_DIR_NONE);
    lv_obj_set_scrollable(paid, false);
    lv_obj_set_clickable(paid, false);
    g_wui.paid_box = paid;

    int pw = QR_F_W - 8;
    g_wui.paid_title = line(paid, "", 0, 80, pw, &pda_font_mono_16, PDA_MSG_ACCENT);
    center_line(g_wui.paid_title);
    g_wui.paid_amt = line(paid, "", 0, 120, pw, &pda_font_mono_14, PDA_TEXT);
    center_line(g_wui.paid_amt);
    g_wui.paid_note = line(paid, "", 0, 150, pw, &pda_font_mono_10, PDA_MSG_TIME);
    center_line(g_wui.paid_note);
    g_wui.paid_bal = line(paid, "", 0, 172, pw, &pda_font_mono_10, PDA_MSG_SYS);
    center_line(g_wui.paid_bal);
    lv_obj_set_hidden(paid, true);

    /* --- what the code says, and what it is doing --- */
    g_wui.invoice_lbl = line_dot(p, "", COL_X, COL_INV_Y, COL_W,
                                 &pda_font_mono_10, PDA_MSG_SYS);
    g_wui.expires_lbl = line_dot(p, "", COL_X, COL_EXP_Y, COL_W,
                                 &pda_font_mono_10, PDA_MSG_TIME);
    g_wui.status_lbl = line_dot(p, "", COL_X, COL_STA_Y, COL_W,
                                &pda_font_mono_14, PDA_MSG_TEXT);

    live_button(p, tr(TR_WALLET_NEW_REQUEST), COL_X, NEWRQ_Y, NEWRQ_W, NEWRQ_H,
                on_new_request, NULL, g_wui.group);

    grit_rule(p, COL_X, COL_RULE_Y, COL_W, false, PDA_RULE);
    if(wallet_is_mock())
        line_dot(p, tr(TR_WALLET_MOCK_BANNER), COL_X, COL_MOCK_Y, COL_W,
                 &pda_font_mono_10, PDA_MSG_ACCENT);
    g_wui.hint = line_dot(p, tr(TR_WALLET_QR_HINT), COL_X, COL_HINT_Y, COL_W,
                          &pda_font_mono_10, PDA_MSG_TIME);

    /* --- what it is showing right now --- */
    /* wired before the first look: setting the textarea's text above must
       not make a second invoice, and typing a memo has to make one */
    lv_obj_add_event_cb(g_wui.memo, on_memo, LV_EVENT_VALUE_CHANGED, NULL);
    show_invoice();
}

/* ---------- SEND ---------- */

static void build_send(lv_obj_t * p)
{
    build_sub_title(p, TR_SEND, true);
    load_env_invoice();

    line(p, tr(TR_WALLET_INVOICE), SEND_FLD_X, SEND_LBL_Y, 300,
         &pda_font_mono_10, PDA_MSG_SYS);
    g_wui.send_inv = field(p, SEND_FLD_X, SEND_INV_Y, SEND_FLD_W, SEND_INV_H,
                           &pda_font_mono_10, false, s_invoice,
                           (int)(sizeof(s_invoice) - 1));

    line(p, tr(TR_WALLET_MEMO), SEND_FLD_X, SEND_MEM_Y, 300,
         &pda_font_mono_10, PDA_MSG_SYS);
    g_wui.send_memo = field(p, SEND_FLD_X, SEND_MEMO_Y, SEND_FLD_W, SEND_MEMO_H,
                            &pda_font_mono_14, true, s_pay_memo,
                            (int)(sizeof(s_pay_memo) - 1));

    grit_rule(p, 6, SEND_RULE_Y, 788, false, PDA_RULE);

    live_button(p, tr(TR_WALLET_CHECK), SEND_CHECK_X, SEND_BTN_Y,
                SEND_CHECK_W, SEND_BTN_H, on_check, NULL, g_wui.group);
    dim_button(p, tr(TR_WALLET_SCAN), SEND_SCAN_X, SEND_BTN_Y, SEND_SCAN_W);
    why_line(p, tr(TR_WALLET_WHY_CAMERA), SEND_SCAN_X, SEND_WHY_Y, SEND_SCAN_W);

    g_wui.send_status = line(p, "", SEND_FLD_X, SEND_ST_Y, SEND_FLD_W,
                             &pda_font_mono_14, PDA_CARRIER_DOWN);
    lv_label_set_long_mode(g_wui.send_status, LV_LABEL_LONG_WRAP);
    lv_obj_set_height(g_wui.send_status, 36);

    line(p, tr(TR_WALLET_SEND_HINT), SEND_FLD_X, SEND_HINT_Y, SEND_FLD_W,
         &pda_font_mono_10, PDA_MSG_TIME);

    if(s_send_err[0]) set_send_status(s_send_err, false);

    lv_obj_add_event_cb(g_wui.send_memo, on_pay_memo, LV_EVENT_VALUE_CHANGED, NULL);
}

/* ---------- CONFIRM PAY ---------- */

static void build_confirm(lv_obj_t * p)
{
    build_sub_title(p, TR_WALLET_CONFIRM, false);

    char num[32], buf[64];

    /* the payment went through: the numbers moved, here is where they are */
    if(s_sent) {
        lv_obj_t * t = line(p, tr(TR_WALLET_SENT), 0, SENT_TITLE_Y, BACK_W,
                            &pda_font_mono_16, PDA_MSG_ACCENT);
        center_line(t);

        /* the fee is re-read after PAY, not only cached from CHECK (§6):
           the mock always knew its 1-sat fee, nwc answers the one the pay
           response carried - and -1 means no combined total is shown, only
           the amount actually paid, with the honest note below */
        int64_t fee = wallet_fee_sats(s_confirm_sats);
        format_sats(fee >= 0 ? s_confirm_sats + fee : s_confirm_sats,
                    num, sizeof(num));
        snprintf(buf, sizeof(buf), "- %s %s", num, tr(TR_WALLET_SATS));
        lv_obj_t * a = line(p, buf, 0, SENT_AMT_Y, BACK_W, &pda_font_mono_14, PDA_TEXT);
        center_line(a);

        const char * note = wallet_is_mock() ? tr(TR_WALLET_PAID_NOTE)
                            : (fee < 0 ? tr(TR_WALLET_FEE_NOTE) : "");
        lv_obj_t * n = line(p, note, 0, SENT_NOTE_Y, BACK_W,
                            &pda_font_mono_10, PDA_MSG_TIME);
        center_line(n);

        format_sats(wallet_balance_sats(), num, sizeof(num));
        snprintf(buf, sizeof(buf), "%s %s %s", tr(TR_WALLET_BALANCE),
                 num, tr(TR_WALLET_SATS));
        lv_obj_t * b = line(p, buf, 0, SENT_BAL_Y, BACK_W,
                            &pda_font_mono_10, PDA_MSG_SYS);
        center_line(b);

        live_button(p, tr(TR_WALLET_BACK), (BACK_W - CONF_BTN_W) / 2, SENT_BTN_Y,
                    CONF_BTN_W, CONF_BTN_H, on_sent_done, NULL, g_wui.group);
        return;
    }

    line(p, tr(TR_WALLET_INVOICE), CONF_K_X, CONF_LBL_Y, 300,
         &pda_font_mono_10, PDA_MSG_SYS);
    line_dot(p, s_invoice, CONF_K_X, CONF_INV_Y, SEND_FLD_W,
             &pda_font_mono_10, PDA_MSG_TEXT);

    line(p, tr(TR_WALLET_MEMO), CONF_K_X, CONF_MEM_Y, 300,
         &pda_font_mono_10, PDA_MSG_SYS);
    line_dot(p, s_pay_memo[0] ? s_pay_memo : "-", CONF_K_X, CONF_MEMO_Y, SEND_FLD_W,
             &pda_font_mono_14, PDA_MSG_TEXT);

    grit_rule(p, 6, CONF_RULE_Y, 788, false, PDA_RULE);

    line(p, tr(TR_WALLET_AMOUNT), CONF_K_X, CONF_AMT_Y, CONF_K_W,
         &pda_font_mono_10, PDA_MSG_SYS);
    format_sats(s_confirm_sats, num, sizeof(num));
    snprintf(buf, sizeof(buf), "%s %s", num, tr(TR_WALLET_SATS));
    line(p, buf, CONF_V_X, CONF_AMT_Y - 5, CONF_V_W, &pda_font_mono_16, PDA_TEXT);

    line(p, tr(TR_WALLET_FEE), CONF_K_X, CONF_FEE_Y, CONF_K_W,
         &pda_font_mono_10, PDA_MSG_SYS);
    if(s_confirm_fee < 0) {
        /* -1 = unknown (the seam's fee contract, section 6): the wallet
           takes its fee when the payment goes through - honest words where
           the number would be, never a made-up one */
        line_dot(p, tr(TR_WALLET_FEE_NOTE), CONF_V_X, CONF_FEE_Y - 4,
                 CONF_V_W, &pda_font_mono_10, PDA_MSG_TIME);
    }
    else {
        format_sats(s_confirm_fee, num, sizeof(num));
        snprintf(buf, sizeof(buf), "%s %s", num, tr(TR_WALLET_SATS));
        line(p, buf, CONF_V_X, CONF_FEE_Y - 4, CONF_V_W, &pda_font_mono_14, PDA_MSG_TEXT);
    }

    grit_rule(p, 6, CONF_RULE2_Y, 788, false, PDA_RULE);

    g_wui.conf_status = line(p, "", CONF_K_X, CONF_ST_Y, SEND_FLD_W,
                             &pda_font_mono_14, PDA_CARRIER_DOWN);
    lv_label_set_long_mode(g_wui.conf_status, LV_LABEL_LONG_WRAP);
    lv_obj_set_height(g_wui.conf_status, 36);

    live_button(p, tr(TR_WALLET_CANCEL), CONF_CANC_X, CONF_BTN_Y, CONF_BTN_W,
                CONF_BTN_H, on_cancel, NULL, g_wui.group);
    live_button(p, tr(TR_WALLET_PAY), CONF_PAY_X, CONF_BTN_Y, CONF_BTN_W,
                CONF_BTN_H, on_pay, NULL, g_wui.group);

    if(wallet_is_mock())
        line_dot(p, tr(TR_WALLET_MOCK_BANNER), CONF_K_X, CONF_MOCK_Y, SEND_FLD_W,
                 &pda_font_mono_10, PDA_MSG_ACCENT);
}

/* ---------- HISTORY ---------- */

static void hist_row(lv_obj_t * list, const wallet_tx_t * tx, int i)
{
    lv_obj_t * row = lv_obj_create(list);
    lv_obj_set_pos(row, 0, i * HIST_ROW_H);
    lv_obj_set_size(row, HIST_LIST_W - 12, HIST_ROW_H);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(row, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(row, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(row, LV_DIR_NONE);
    lv_obj_set_scrollable(row, false);
    lv_obj_set_clickable(row, false);

    /* which way the money went, then the amount signed to match */
    line(row, tr(tx->outgoing ? TR_WALLET_DIR_OUT : TR_WALLET_DIR_IN),
         H_DIR_X, 5, H_DIR_W, &pda_font_mono_10,
         tx->outgoing ? PDA_MSG_SYS : PDA_MSG_ACCENT);

    char num[32], buf[48];
    format_sats(tx->sats, num, sizeof(num));
    snprintf(buf, sizeof(buf), "%s%s", tx->outgoing ? "-" : "+", num);
    line_dot(row, buf, H_AMT_X, 5, H_AMT_W, &pda_font_mono_10, PDA_MSG_TEXT);

    line_dot(row, tx->memo, H_MEMO_X, 5, H_MEMO_W, &pda_font_mono_10, PDA_MSG_TIME);

    struct tm when;
    localtime_r(&tx->when, &when);
    char date[16];
    snprintf(date, sizeof(date), "%02d-%02d", when.tm_mon + 1, when.tm_mday);
    lv_obj_t * d = line(row, date, H_DATE_X, 5, H_DATE_W, &pda_font_mono_10, PDA_MSG_TIME);
    lv_obj_set_style_text_align(d, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);

    lv_obj_t * st = line(row, tr(tx->settled ? TR_WALLET_PAID : TR_WALLET_ST_EXPIRED),
                         H_STAT_X, 5, H_STAT_W, &pda_font_mono_10,
                         tx->settled ? PDA_MSG_ACCENT : PDA_CARRIER_DOWN);
    lv_obj_set_style_text_align(st, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);

    grit_rule(row, 0, HIST_ROW_H - 1, lv_pct(100), false, PDA_RULE);
}

static void build_history(lv_obj_t * p)
{
    build_sub_title(p, TR_WALLET_HISTORY, true);

    lv_obj_t * list = lv_obj_create(p);
    lv_obj_set_pos(list, HIST_LIST_X, HIST_LIST_Y);
    lv_obj_set_size(list, HIST_LIST_W, HIST_LIST_H);
    lv_obj_set_style_bg_color(list, lv_color_hex(PDA_CHAT_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(list, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(list, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(list, lv_color_hex(PDA_CHAT_EDGE), LV_PART_MAIN);
    lv_obj_set_style_radius(list, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_all(list, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_row(list, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(list, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);
    lv_obj_set_scrollable(list, true);
    lv_obj_set_style_bg_color(list, lv_color_hex(PDA_SCROLL), LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_opa(list, LV_OPA_COVER, LV_PART_SCROLLBAR);
    lv_obj_set_style_width(list, 4, LV_PART_SCROLLBAR);
    lv_obj_set_style_radius(list, 0, LV_PART_SCROLLBAR);
    g_wui.hist_list = list;
    /* in the keypad group so the Pi's arrows scroll it once it has focus */
    if(g_wui.group) lv_group_add_obj(g_wui.group, list);

    wallet_tx_t rows[HIST_MAX];
    int n = wallet_history(rows, HIST_MAX);
    s_hist_top_valid = false;

    if(n <= 0) {
        lv_obj_t * e = line(list, tr(TR_WALLET_NO_HISTORY), 0, 8,
                            HIST_LIST_W - 16, &pda_font_mono_14, PDA_MSG_TIME);
        center_line(e);
        return;
    }

    s_hist_top = rows[0];
    s_hist_top_valid = true;
    for(int i = 0; i < n; i++) hist_row(list, &rows[i], i);
}

/* A payment can land while this screen is up - an invoice made on REQUEST
 * is settled by the same clock. Rebuild only when the newest row is not
 * the one already on screen, so focus is not thrown away every tick. */
static void refresh_history(void)
{
    if(!g_wui.content || !g_wui.hist_list) return;

    wallet_tx_t top;
    int n = wallet_history(&top, 1);

    bool same = ((n > 0) == s_hist_top_valid);
    if(same && n > 0) {
        same = top.outgoing == s_hist_top.outgoing
            && top.sats == s_hist_top.sats
            && top.when == s_hist_top.when
            && top.settled == s_hist_top.settled
            && strcmp(top.memo, s_hist_top.memo) == 0;
    }
    if(same) return;
    rebuild();
}

/* ---------- the panel ---------- */

static void build_current(void)
{
    if(!g_wui.root) return;
    /* no backend, no screens below the balance - whichever one was up */
    if(wallet_state() != WALLET_READY) s_screen = SCREEN_MAIN;

    lv_obj_t * c = lv_obj_create(g_wui.root);
    lv_obj_set_pos(c, 0, 0);
    lv_obj_set_size(c, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(c, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(c, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(c, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(c, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(c, LV_DIR_NONE);
    lv_obj_set_scrollable(c, false);
    lv_obj_set_clickable(c, false);     /* only the widgets take the click */
    g_wui.content = c;

    if(s_screen == SCREEN_REQUEST) build_request(c);
    else if(s_screen == SCREEN_SEND) build_send(c);
    else if(s_screen == SCREEN_CONFIRM) build_confirm(c);
    else if(s_screen == SCREEN_HISTORY) build_history(c);
    else switch(wallet_state()) {
        case WALLET_READY: build_ready(c); break;
        case WALLET_ERROR: build_error(c); break;
        case WALLET_UNSET:
        default:           build_unset(c); break;
    }
}

/* take the screen down: animations first, then the tree, then the pixels
   the canvas was holding */
static void content_clear(void)
{
    if(g_wui.content) {
        lv_kill_tree(g_wui.content);
        lv_obj_delete(g_wui.content);
    }
    g_wui.content = NULL;
    if(g_wui.qr_px) {
        free(g_wui.qr_px);
        g_wui.qr_px = NULL;
    }
    g_wui.amount_lbl = NULL;
    g_wui.memo = NULL;
    g_wui.frame = NULL;
    g_wui.qr = NULL;
    g_wui.qr_err = NULL;
    g_wui.paid_box = NULL;
    g_wui.paid_title = NULL;
    g_wui.paid_amt = NULL;
    g_wui.paid_note = NULL;
    g_wui.paid_bal = NULL;
    g_wui.invoice_lbl = NULL;
    g_wui.expires_lbl = NULL;
    g_wui.status_lbl = NULL;
    g_wui.newrq = NULL;
    g_wui.hint = NULL;
    g_wui.inv_state = -1;
    g_wui.send_inv = NULL;
    g_wui.send_memo = NULL;
    g_wui.send_status = NULL;
    g_wui.conf_status = NULL;
    g_wui.hist_list = NULL;
    s_hist_top_valid = false;
    s_conn_ta = NULL;
    s_conn_btn_lbl = NULL;
}

lv_obj_t * wallet_ui_create(lv_obj_t * parent, lv_group_t * group)
{
    /* a panel that was rebuilt without a destroy() would drop its QR buffer
       here; nothing does, but the pointer is checked before it is cleared */
    if(g_wui.qr_px) {
        free(g_wui.qr_px);
        g_wui.qr_px = NULL;
    }
    memset(&g_wui, 0, sizeof(g_wui));
    g_wui.group = group;
    g_wui.inv_state = -1;

    wallet_init();              /* idempotent: the shell calls it too */
    s_seen_state = (int)wallet_state();   /* the first tick has no change */

    lv_obj_t * p = lv_obj_create(parent);
    lv_obj_set_pos(p, 0, 0);
    lv_obj_set_size(p, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(p, lv_color_hex(PDA_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(p, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(p, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(p, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(p, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(p, LV_DIR_NONE);
    lv_obj_set_scrollable(p, false);
    g_wui.root = p;

    /* the gritty surface everything sits on (the grain can be off) */
    grit_backdrop(p, BACK_W, BACK_H);

    build_current();

    /* the timer goes on last: it must not outlive the screen it reads, and
       destroy() takes it down before the tree it points at goes */
    g_wui.tick = lv_timer_create(wallet_tick, 250, NULL);
    return p;
}

void wallet_ui_destroy(void)
{
    if(g_wui.tick) {
        lv_timer_delete(g_wui.tick);
        g_wui.tick = NULL;
    }
    if(g_wui.root) {
        /* animations first: one that outlives its object is a crash */
        lv_kill_tree(g_wui.root);
        /* then the tree itself - which also takes its widgets out of the
           keypad group (the shell does not delete this pointer again) */
        lv_obj_delete(g_wui.root);
    }
    if(g_wui.qr_px) free(g_wui.qr_px);
    memset(&g_wui, 0, sizeof(g_wui));
    s_seen_state = -1;          /* a fresh panel re-reads the state */
    s_conn_err[0] = 0;
    s_conn_armed = false;
}

/* ---------- the way in from outside ---------- */

/* The merchant card's [REQUEST] button, and later anything else that has a
 * name to ask for a payment with: this is the Wallet tab, on the screen
 * that makes the invoice, with what it is for already written in - and an
 * invoice for it, made right here, so the merchant can scan straight away.
 *
 * With no backend build_current() puts the honest "not set up" screen up
 * instead (the memo is kept, so it is still there when one arrives), and
 * with a backend that cannot answer yet nothing is faked: the screens are
 * built from wallet.h and nothing here assumes the mock. */
void wallet_open_with_memo(const char * memo)
{
    snprintf(s_memo, sizeof(s_memo), "%s", memo ? memo : "");
    if(wallet_state() == WALLET_READY) make_invoice_now();
    s_screen = SCREEN_REQUEST;

    if(g_wui.root) {
        content_clear();
        build_current();
    }
    pda_shell_show_wallet();     /* the tab it lives on is the shell's */
}
