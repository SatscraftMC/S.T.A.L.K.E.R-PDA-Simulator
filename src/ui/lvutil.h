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

/* a carrier's colour: green when it is up, grey while it tries, red when it
   is not there */
static inline uint32_t lv_carrier_color(chat_carrier_state_t st)
{
    if(st == CHAT_CARRIER_ONLINE) return PDA_CARRIER_UP;
    if(st == CHAT_CARRIER_CONNECTING) return PDA_CARRIER_WAIT;
    return PDA_CARRIER_DOWN;
}

#endif
