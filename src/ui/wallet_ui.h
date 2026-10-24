#ifndef PDA_WALLET_UI_H
#define PDA_WALLET_UI_H

#include "lvgl/lvgl.h"

/* The Wallet tab: the honest states first - "not set up" when no backend is
 * installed, the balance screen when one is (the mock, for now), the error
 * screen when PDA_WALLET_BACKEND asked for something that does not exist -
 * then REQUEST, where the backend's invoice becomes a QR code, SEND, where
 * an invoice from a paste or from PDA_INVOICE is offered for payment,
 * CONFIRM, where its amount and fee are read back before PAY will move
 * anything, and HISTORY, the rows the backend kept. */

lv_obj_t * wallet_ui_create(lv_obj_t * parent, lv_group_t * group);

/* the panel is rebuilt when the theme or the language changes: the old one
   has to let go of its animations and objects first */
void wallet_ui_destroy(void);

/* Take the device to the Wallet tab and open REQUEST with memo already
   written in it - the merchant card's [REQUEST] button does this, and so
   will anything else that has a name to ask for a payment with (the
   Contacts list, a real backend's "pay this" entry point). The tab comes
   with it: this is the one call a screen outside the Wallet makes.
   With no backend the tab still opens, on the honest "not set up" screen,
   and the memo waits there for one. */
void wallet_open_with_memo(const char * memo);

#endif
