#ifndef PDA_RADIO_UI_H
#define PDA_RADIO_UI_H

#include "lvgl/lvgl.h"

/* The Radio tab: simple MP3 player.
 * Scans a directory for MP3 files and plays them. */

lv_obj_t * radio_ui_create(lv_obj_t * parent, lv_group_t * group);

/* the panel is rebuilt when the theme or the language changes */
void radio_ui_destroy(void);

/* set the directory to scan for MP3s */
void radio_ui_set_music_dir(const char * dir);

#endif