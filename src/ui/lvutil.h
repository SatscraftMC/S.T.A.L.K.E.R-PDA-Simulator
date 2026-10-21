#ifndef PDA_LVUTIL_H
#define PDA_LVUTIL_H

#include "lvgl/lvgl.h"
#include "chat/chat.h"
#include "ui/palette.h"

/* Deleting an object that has a running animation leaves that animation
   pointing at freed memory - and the default theme puts a transition on every
   button, so a button that was just pressed has one. The animation has to go
   first. */

static inline void lv_kill_anim(lv_obj_t * obj)
{
    if(obj) lv_anim_delete(obj, NULL);
}

/* cancel the animations of an object and everything under it */
static inline void lv_kill_tree(lv_obj_t * obj)
{
    if(!obj) return;
    uint32_t n = lv_obj_get_child_count(obj);
    for(uint32_t i = 0; i < n; i++) lv_kill_tree(lv_obj_get_child(obj, i));
    lv_anim_delete(obj, NULL);
}

/* The default theme puts a transition on every button. A transition is an
   animation, and an animation that is still running when its object is
   deleted carries on with freed memory - which crashes the app. This gives a
   button a transition with nothing to animate: no animation, nothing that can
   outlive the object. */
static inline void lv_no_transition(lv_obj_t * obj)
{
    static const lv_style_prop_t no_props[1] = { 0 };
    static lv_style_transition_dsc_t tr;
    static bool ready = false;
    if(!ready) {
        lv_style_transition_dsc_init(&tr, no_props, lv_anim_path_linear, 0, 0, NULL);
        ready = true;
    }
    lv_obj_set_style_transition(obj, &tr, LV_PART_MAIN);
    lv_obj_set_style_transition(obj, &tr, LV_STATE_PRESSED);
}
/* a carrier's colour: green when it is up, grey while it tries, red when
   it is not there */
static inline uint32_t lv_carrier_color(chat_carrier_state_t st)
{
    if(st == CHAT_CARRIER_ONLINE) return PDA_CARRIER_UP;
    if(st == CHAT_CARRIER_CONNECTING) return PDA_CARRIER_WAIT;
    return PDA_CARRIER_DOWN;
}

/* ---------- the caret ----------
 *
 * LVGL draws its own text cursor *under* the text: the text of a field is a
 * label, a child of the field, and a child is drawn after its parent. The
 * letters of this font are wider than their advance, so LVGL's cursor always
 * hides under a letter - it is never seen.
 *
 * So the caret is a bar of our own, on top of the text, and it follows the
 * cursor. It blinks while the field has the focus, and the placeholder steps
 * aside while the field is being edited (the bar would stand on its first
 * letter).
 */

#define LV_CARET_W   2       /* the width of the bar */
#define LV_CARET_MS  500     /* how long the bar is on, and off */

static inline void lv_caret_blink_cb(void * obj, int32_t v)
{
    lv_obj_set_style_opa(obj, (lv_opa_t)v, LV_PART_MAIN);
}

static inline void lv_caret_gone(lv_event_t * e)
{
    /* the blink must not outlive the bar */
    lv_anim_delete(lv_event_get_current_target(e), NULL);
}

/* the bar itself: a child of the label, so it is drawn over the text */
static inline lv_obj_t * lv_caret_bar(lv_obj_t * ta)
{
    lv_obj_t * label = lv_obj_get_child(ta, 0);
    if(!label) return NULL;
    return lv_obj_get_child(label, 0);
}

/* put the bar where the next character goes */
static inline void lv_caret_place(lv_obj_t * ta)
{
    lv_obj_t * label = lv_obj_get_child(ta, 0);
    lv_obj_t * caret = label ? lv_obj_get_child(label, 0) : NULL;
    if(!label || !caret) return;

    /* the position is relative to the label's content, which is where the
       bar lives too */
    lv_point_t p;
    lv_label_get_letter_pos(label, lv_textarea_get_cursor_pos(ta), &p);
    lv_obj_set_pos(caret, p.x, p.y);
}

/* (re)start the blink: the bar is on when it starts */
static inline void lv_caret_blink(lv_obj_t * ta)
{
    lv_obj_t * caret = lv_caret_bar(ta);
    if(!caret) return;

    lv_anim_delete(caret, NULL);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, caret);
    lv_anim_set_exec_cb(&a, lv_caret_blink_cb);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_duration(&a, LV_CARET_MS);
    /* without the reverse the step path would hold the value for good */
    lv_anim_set_reverse_duration(&a, LV_CARET_MS);
    lv_anim_set_path_cb(&a, lv_anim_path_step);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&a);
}

/* put the bar where the next character goes, and light it up again: the
   caret is solid while the text is being changed */
static inline void lv_caret_refresh(lv_obj_t * ta)
{
    lv_caret_place(ta);
    lv_caret_blink(ta);
}

static inline void lv_caret_focus_cb(lv_event_t * e)
{
    lv_obj_t * ta = lv_event_get_current_target(e);
    if(!lv_caret_bar(ta)) return;

    /* the placeholder steps aside while the field is being edited: the bar
       would stand on its first letter */
    lv_obj_set_style_text_opa(ta, LV_OPA_TRANSP, LV_PART_TEXTAREA_PLACEHOLDER);
    lv_caret_refresh(ta);
}

static inline void lv_caret_defocus_cb(lv_event_t * e)
{
    lv_obj_t * ta = lv_event_get_current_target(e);
    lv_obj_t * caret = lv_caret_bar(ta);

    lv_obj_set_style_text_opa(ta, LV_OPA_COVER, LV_PART_TEXTAREA_PLACEHOLDER);
    if(caret) {
        lv_anim_delete(caret, NULL);
        lv_obj_set_style_opa(caret, LV_OPA_TRANSP, LV_PART_MAIN);
    }
}

static inline void lv_caret_value_cb(lv_event_t * e)
{
    lv_caret_refresh(lv_event_get_current_target(e));
}

static inline void lv_caret_key_cb(lv_event_t * e)
{
    /* the arrow keys move the cursor without an event of their own */
    lv_caret_refresh(lv_event_get_current_target(e));
}

static inline void lv_caret_press_cb(lv_event_t * e)
{
    /* a click puts the cursor where the mouse is */
    lv_caret_refresh(lv_event_get_current_target(e));
}

/* give a text field a caret */
static inline void lv_caret_init(lv_obj_t * ta)
{
    /* LVGL's own cursor is hidden: it is drawn under the text and can never
       be seen - the bar below is the one that shows */
    lv_obj_set_style_bg_opa(ta, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_set_style_border_opa(ta, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_set_style_text_color(ta, lv_color_hex(PDA_MSG_TEXT), LV_PART_CURSOR);

    lv_obj_t * label = lv_obj_get_child(ta, 0);
    if(!label) return;

    const lv_font_t * font = lv_obj_get_style_text_font(ta, LV_PART_MAIN);
    lv_obj_t * caret = lv_obj_create(label);
    lv_obj_set_size(caret, LV_CARET_W, lv_font_get_line_height(font));
    lv_obj_set_style_bg_color(caret, lv_color_hex(PDA_MSG_ACCENT), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(caret, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(caret, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(caret, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(caret, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(caret, 0, LV_PART_MAIN);
    lv_obj_set_style_outline_width(caret, 0, LV_PART_MAIN);
    /* no cursor until the field has the focus */
    lv_obj_set_style_opa(caret, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_clickable(caret, false);
    lv_obj_set_scrollbar_mode(caret, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scrollable(caret, false);
    lv_obj_add_event_cb(caret, lv_caret_gone, LV_EVENT_DELETE, NULL);
    lv_obj_set_pos(caret, 0, 0);

    lv_obj_add_event_cb(ta, lv_caret_focus_cb, LV_EVENT_FOCUSED, NULL);
    lv_obj_add_event_cb(ta, lv_caret_defocus_cb, LV_EVENT_DEFOCUSED, NULL);
    lv_obj_add_event_cb(ta, lv_caret_value_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(ta, lv_caret_key_cb, LV_EVENT_KEY, NULL);
    lv_obj_add_event_cb(ta, lv_caret_press_cb, LV_EVENT_PRESSED, NULL);
}

#endif

