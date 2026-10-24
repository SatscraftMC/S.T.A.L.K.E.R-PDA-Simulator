#include "ui/wallet_ui.h"
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
 * Two screens today, four when the plan is done: the balance (which is also
 * where "not set up" and the error live), and REQUEST - an amount, a memo
 * and a QR code of a bolt11 the backend just made. The buttons that need a
 * screen this build does not have are there to be seen and say why they
 * cannot be pressed; nothing here pretends to work.
 *
 *   MAIN                                REQUEST
 *   +--------------------------------+  +--------------------------------+
 *   | WALLET   MOCK - NO REAL MONEY  |  | REQUEST             [ BACK ]   |
 *   |        1 234 567               |  | AMOUNT  500 sats [-10][-1]...  |
 *   |            sats                |  | MEMO    [ coffee           ]   |
 *   |      0.01234567 BTC            |  | +-----------------+ lnbc50n1p..|
 *   | STATE  mock preview            |  | |  # # #  # #     | expires   |
 *   | NET    not connected           |  | |  #  # #   #     | 04:59     |
 *   |--------------------------------|  | +-----------------+ waiting...|
 *   | [REQUEST] [SEND] [HISTORY] ... |  |             [ NEW REQUEST ]    |
 *   +--------------------------------+  +--------------------------------+
 *
 * Everything the REQUEST screen shows comes from wallet.h: it makes an
 * invoice, reads it back, and is polled for the moment the backend says it
 * was paid. The mock behind that settles on a timer, so the PAID screen can
 * be seen before a real backend exists. */

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

enum {
    SCREEN_MAIN = 0,
    SCREEN_REQUEST = 1
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
} wallet_ui_t;

static wallet_ui_t g_wui;

/* these outlive the panel: a theme change rebuilds the screen you were on,
   with the amount and the memo you had typed */
static int s_screen = SCREEN_MAIN;
static int64_t s_amount = 500;
static char s_memo[48];
static char s_last_err[192];

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

/* a button that is there to be seen, not to be pressed: disabled, outside
 * the keypad group, with its reason underneath */
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
    lv_obj_set_style_text_color(l, lv_color_hex(PDA_MSG_TIME), LV_PART_MAIN);
    lv_obj_set_style_pad_all(l, 0, LV_PART_MAIN);

    lv_obj_add_state(b, LV_STATE_DISABLED);
    lv_obj_set_clickable(b, false);
}

/* the reason a dimmed button is dimmed, in its own column under it */
static void why_line(lv_obj_t * p, const char * txt, int x, int y, int w)
{
    lv_obj_t * l = line(p, txt, x, y, w, &pda_font_mono_10, PDA_MSG_TIME);
    center_line(l);
}

/* a button that works: in the keypad group, accent edge and accent label */
static void live_button(lv_obj_t * p, const char * txt, int x, int y, int w, int h,
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
}

/* ---------- forward declarations ---------- */

static void on_open_request(lv_event_t * e);
static void on_back(lv_event_t * e);
static void on_new_request(lv_event_t * e);
static void on_step(lv_event_t * e);
static void on_memo(lv_event_t * e);
static void build_current(void);
static void content_clear(void);
static void regenerate(void);
static void refresh_request(void);

/* ---------- the three states ---------- */

/* no backend at all - this is what the device honestly is today */
static void build_unset(lv_obj_t * p)
{
    build_title(p, NULL);

    lv_obj_t * h = line(p, tr(TR_WALLET_NOT_SET_UP), 0, 140, BACK_W,
                        &pda_font_mono_16, PDA_MSG_ACCENT);
    center_line(h);

    lv_obj_t * b = line(p, tr(TR_WALLET_NO_BACKEND), 120, 182, 560,
                        &pda_font_mono_14, PDA_TEXT);
    center_block(b);

    dim_button(p, tr(TR_WALLET_CREATE), 236, 262, 156);
    dim_button(p, tr(TR_WALLET_RESTORE), 408, 262, 156);

    lv_obj_t * why = line(p, tr(TR_WALLET_COMING), 0, 312, BACK_W,
                          &pda_font_mono_10, PDA_MSG_TIME);
    center_line(why);
}

/* a backend is up: the balance screen (today only the mock ever gets here) */
static void build_ready(lv_obj_t * p)
{
    build_title(p, tr(TR_WALLET_MOCK_BANNER));

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
    line(p, tr(TR_WALLET_MOCK_STATE), 340, 184, 300,
         &pda_font_mono_10, PDA_MSG_ACCENT);

    lv_obj_t * k2 = line(p, tr(TR_WALLET_NET), 250, 208, 70,
                         &pda_font_mono_10, PDA_MSG_SYS);
    lv_obj_set_style_text_align(k2, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    line(p, tr(TR_WALLET_NET_OFF), 340, 208, 300,
         &pda_font_mono_10, PDA_MSG_TEXT);

    /* the actions. Only one of them has a screen behind it in this build;
       the others say what they are waiting for instead of looking broken */
    grit_rule(p, 6, MAIN_RULE_Y, 788, false, PDA_RULE);

    live_button(p, tr(TR_WALLET_REQUEST), MAIN_BTN_X0, MAIN_BTN_Y,
                MAIN_BTN_W, MAIN_BTN_H, on_open_request, NULL, g_wui.group);

    int x1 = MAIN_BTN_X0 + (MAIN_BTN_W + MAIN_BTN_GAP);
    int x2 = x1 + (MAIN_BTN_W + MAIN_BTN_GAP);
    int x3 = x2 + (MAIN_BTN_W + MAIN_BTN_GAP);

    dim_button(p, tr(TR_SEND), x1, MAIN_BTN_Y, MAIN_BTN_W);
    why_line(p, tr(TR_WALLET_WHY_NEXT), x1, MAIN_WHY_Y, MAIN_BTN_W);

    dim_button(p, tr(TR_WALLET_HISTORY), x2, MAIN_BTN_Y, MAIN_BTN_W);
    why_line(p, tr(TR_WALLET_WHY_NEXT), x2, MAIN_WHY_Y, MAIN_BTN_W);

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
        snprintf(s_last_err, sizeof(s_last_err), "%s", "could not make an invoice");
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
        lv_label_set_text(g_wui.paid_note, tr(TR_WALLET_PAID_NOTE));
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

/* the mock settles on the wall clock, so the panel is polled for it */
static void wallet_tick(lv_timer_t * t)
{
    (void)t;
    if(s_screen != SCREEN_REQUEST) return;
    wallet_poll();
    refresh_request();
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

/* the title the REQUEST screen wears: the banner stays, BACK takes the
   right-hand end of the strip where the balance screen has nothing */
static void build_request_title(lv_obj_t * p)
{
    grit_square(p, 22, 9, 8, PDA_MSG_ACCENT);
    grit_square(p, 770, 9, 8, PDA_MSG_ACCENT);

    lv_obj_t * t = line(p, tr(TR_WALLET_REQUEST), 0, 5, BACK_W,
                        &pda_font_mono_16, PDA_MSG_TEXT);
    center_line(t);

    lv_obj_t * b = line(p, tr(TR_WALLET_MOCK_BANNER), 430, 8, 240,
                        &pda_font_mono_10, PDA_MSG_ACCENT);
    lv_label_set_long_mode(b, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(b, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);

    live_button(p, tr(TR_WALLET_BACK), BACK_BTN_X, BACK_BTN_Y,
                BACK_BTN_W, BACK_BTN_H, on_back, NULL, g_wui.group);

    grit_rule(p, 6, TITLE_RULE_Y, 788, false, PDA_RULE);
}

static void build_request(lv_obj_t * p)
{
    build_request_title(p);

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

    lv_obj_t * ta = lv_textarea_create(p);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_max_length(ta, (uint32_t)(sizeof(s_memo) - 1));
    lv_textarea_set_text(ta, s_memo);
    lv_obj_set_pos(ta, MEM_X, MEM_Y);
    lv_obj_set_size(ta, MEM_W, MEM_H);
    /* the field must not shrink to the size of its text */
    lv_obj_set_style_min_width(ta, MEM_W, LV_PART_MAIN);
    lv_obj_set_style_bg_color(ta, lv_color_hex(PDA_CHAT_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ta, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(ta, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(ta, lv_color_hex(PDA_CHAT_EDGE), LV_PART_MAIN);
    lv_obj_set_style_text_font(ta, &pda_font_mono_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(ta, lv_color_hex(PDA_MSG_TEXT), LV_PART_MAIN);
    lv_obj_set_style_text_color(ta, lv_color_hex(PDA_MSG_SYS), LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_pad_all(ta, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_left(ta, 6, LV_PART_MAIN);
    lv_caret_init(ta);
    g_wui.memo = ta;
    if(g_wui.group) lv_group_add_obj(g_wui.group, ta);

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

/* ---------- the panel ---------- */

static void build_current(void)
{
    if(!g_wui.root) return;
    if(s_screen == SCREEN_REQUEST && wallet_state() != WALLET_READY)
        s_screen = SCREEN_MAIN;

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
}
