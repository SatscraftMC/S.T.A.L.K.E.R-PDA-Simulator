#ifndef PDA_CHAT_UI_H
#define PDA_CHAT_UI_H

#include "lvgl/lvgl.h"

/* The Messages tab: one room, every carrier. */

lv_obj_t * chat_ui_create(lv_obj_t * parent, lv_group_t * group);

/* the panel is rebuilt when the theme or the language changes: the old one
   has to let go of its timer first */
void chat_ui_destroy(void);

/* how often the log is looked at, in milliseconds */
void chat_ui_set_poll(int ms);

/* empty the message log */
void chat_ui_clear(void);

#endif
