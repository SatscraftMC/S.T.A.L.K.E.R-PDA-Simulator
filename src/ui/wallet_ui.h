#ifndef PDA_WALLET_UI_H
#define PDA_WALLET_UI_H

#include "lvgl/lvgl.h"

/* The Wallet tab: the honest states first - "not set up" when no backend is
 * installed, the balance screen when one is (the mock, for now), the error
 * screen when PDA_WALLET_BACKEND asked for something that does not exist -
 * and the REQUEST screen, where the backend's invoice becomes a QR code
 * (phase 2; SEND and HISTORY arrive on top of this seam). */

lv_obj_t * wallet_ui_create(lv_obj_t * parent, lv_group_t * group);

/* the panel is rebuilt when the theme or the language changes: the old one
   has to let go of its animations and objects first */
void wallet_ui_destroy(void);

#endif
