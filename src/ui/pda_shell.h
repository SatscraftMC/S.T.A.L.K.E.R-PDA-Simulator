#ifndef PDA_SHELL_H
#define PDA_SHELL_H

#include "lvgl/lvgl.h"

/* which build this is - the app says it when it starts, so a stale process
   is easy to spot */
#define PDA_VERSION   "v0.3.1-alpha"

lv_obj_t * pda_shell_create(lv_obj_t * parent, lv_group_t * group);

/* Mouse wheel over the map: zoom one step towards the cursor. (x, y) are
   window coordinates, diff is the wheel delta (positive = scrolling up). */
void pda_shell_wheel(int diff, int x, int y);

/* Right click on the map: put a waypoint down (or take the one that is
   there away). (x, y) are window coordinates. */
void pda_shell_right_click(int x, int y);

/* The theme or the language changed: the panels are built with the colours
   and the words, so they are built again. */
void pda_shell_retheme(void);

/* how often the GPS is looked at, in milliseconds */
void pda_shell_set_gps_poll(int ms);

#endif
