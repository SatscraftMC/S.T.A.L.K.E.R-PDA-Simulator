#ifndef PDA_CONTACTS_UI_H
#define PDA_CONTACTS_UI_H

#include "lvgl/lvgl.h"

/* The Contacts tab: the pubkey-keyed trust list, in two sections.
 *
 *   TRUSTED - the keys you pressed TRUST on (contacts.conf, persisted),
 *             label, short pubkey, [REMOVE], tap the row for the full npub
 *             (compare it against what the other device shows on its
 *             User page - the trust is checkable on the device)
 *   SEEN    - signed speakers heard this session that you have not trusted
 *             yet (session-only), label, short pubkey, [TRUST]
 *
 * Trust is pressed here and nowhere else; nothing in this panel trusts by
 * name, because the name is a costume the key underneath proves. */

lv_obj_t * contacts_ui_create(lv_obj_t * parent, lv_group_t * group);

/* the panel is rebuilt when the theme or the language changes: the old one
   has to let go of its timer and objects first (modelled on
   wallet_ui_destroy, which deletes its own tree - not on chat_ui_destroy,
   which is part of the known retheme leak) */
void contacts_ui_destroy(void);

#endif
