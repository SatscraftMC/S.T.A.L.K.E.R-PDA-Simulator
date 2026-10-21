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

/* LVGL draws the text cursor as a box over the character under it, and
   redraws that character in the cursor's own colour - so a yellow box with
   a yellow letter on it makes the letter disappear. The cursor is a thin
   caret at the left of the character instead, and it stays away while the
   field is empty: there is nothing to point at then, and the placeholder
   must show whole. */

static inline void lv_cursor_update(lv_obj_t * ta)
{
    bool focused = lv_obj_has_state(ta, LV_STATE_FOCUSED);
    bool has_text = lv_textarea_get_text(ta)[0] != '\0';
    bool show = focused && has_text;
    lv_obj_set_style_border_opa(ta, show ? LV_OPA_COVER : LV_OPA_TRANSP, LV_PART_CURSOR);
}

static inline void lv_cursor_focus_cb(lv_event_t * e)
{
    lv_cursor_update(lv_event_get_current_target(e));
}

static inline void lv_cursor_value_cb(lv_event_t * e)
{
    lv_cursor_update(lv_event_get_current_target(e));
}

static inline void lv_cursor_only_when_focused(lv_obj_t * ta)
{
    lv_obj_set_style_bg_color(ta, lv_color_hex(PDA_MSG_ACCENT), LV_PART_CURSOR);
    lv_obj_set_style_bg_opa(ta, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_set_style_border_width(ta, 1, LV_PART_CURSOR);
    lv_obj_set_style_border_side(ta, LV_BORDER_SIDE_LEFT, LV_PART_CURSOR);
    lv_obj_set_style_border_color(ta, lv_color_hex(PDA_MSG_ACCENT), LV_PART_CURSOR);
    lv_obj_set_style_border_opa(ta, LV_OPA_TRANSP, LV_PART_CURSOR);
    /* the same colour as the text: then LVGL leaves the letter alone */
    lv_obj_set_style_text_color(ta, lv_color_hex(PDA_MSG_TEXT), LV_PART_CURSOR);
    lv_obj_add_event_cb(ta, lv_cursor_focus_cb, LV_EVENT_FOCUSED, NULL);
    lv_obj_add_event_cb(ta, lv_cursor_focus_cb, LV_EVENT_DEFOCUSED, NULL);
    lv_obj_add_event_cb(ta, lv_cursor_value_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

#endif

