#ifndef PDA_SHELL_H
#define PDA_SHELL_H

#include "lvgl/lvgl.h"

lv_obj_t * pda_shell_create(lv_obj_t * parent, lv_group_t * group);

/* Mouse wheel over the map: zoom one step towards the cursor. (x, y) are
   window coordinates, diff is the wheel delta (positive = scrolling up). */
void pda_shell_wheel(int diff, int x, int y);

#endif
