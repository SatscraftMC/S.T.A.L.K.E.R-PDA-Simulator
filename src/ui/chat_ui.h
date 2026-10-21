#ifndef PDA_CHAT_UI_H
#define PDA_CHAT_UI_H

#include "lvgl/lvgl.h"

/* The Messages tab: one room, every carrier. */

lv_obj_t * chat_ui_create(lv_obj_t * parent, lv_group_t * group);

#endif
