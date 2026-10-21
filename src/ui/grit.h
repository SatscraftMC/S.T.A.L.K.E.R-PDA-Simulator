#ifndef PDA_GRIT_H
#define PDA_GRIT_H

#include "lvgl/lvgl.h"

/* The STALKER "guide" look: a gritty surface, framed panels with crop
   marks in the corners, thin rules and the small square indicators. */

/* a dark, noisy surface: grain, scanlines, stains and a vignette */
lv_obj_t * grit_backdrop(lv_obj_t * parent, int w, int h);

/* a panel with a double border */
lv_obj_t * grit_frame(lv_obj_t * parent, int x, int y, int w, int h, uint32_t color);

/* the crop marks in the corners of a frame - call them after the frame has
   its own children, so they end up on top of them */
void grit_corners(lv_obj_t * frame, int w, int h, uint32_t color);

/* a thin line - the length may be lv_pct(100) */
lv_obj_t * grit_rule(lv_obj_t * parent, int x, int y, int len, bool vertical,
                     uint32_t color);

/* a small square indicator */
lv_obj_t * grit_square(lv_obj_t * parent, int x, int y, int size, uint32_t color);

#endif
