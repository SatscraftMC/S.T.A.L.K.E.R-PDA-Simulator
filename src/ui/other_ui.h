#ifndef PDA_OTHER_UI_H
#define PDA_OTHER_UI_H

#include "lvgl/lvgl.h"

/* The Other tab: the device's settings, connections and user profile.
   A column of sections on the left, sub-tabs across the top of the pane
   on the right, the page below that. */

lv_obj_t * other_ui_create(lv_obj_t * parent, lv_group_t * group);

/* the panel is rebuilt when the theme or the language changes: the old one
   has to let go of its timer first */
void other_ui_destroy(void);

/* which page and sub page are open - they survive a rebuild */
void other_ui_get_state(int * page, int * sub);
void other_ui_set_state(int page, int sub);

/* The shell pushes the version and the GPS state in here - they belong on
   the System page. */
void other_ui_status(const char * version, const char * gps);

#endif
