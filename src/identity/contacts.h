#ifndef PDA_CONTACTS_H
#define PDA_CONTACTS_H

#include <stdbool.h>

/* The contact book: keyed by the 64-hex public key, never by the name.
   Trust is what the user pressed TRUST on; the label is just what the
   device was called at that moment.

   File: contacts.conf, beside prefs.conf (PDA_PREFS isolates them
   together), one "<64-hex pubkey> = <label>" line per entry, mode 0600,
   written through the shared atomic writer of identity/state_file.c.

   File-first on every change: validate -> write -> only on a successful
   rename does the in-memory table move and contacts_version() bump. A
   press that cannot be honored never reports success, and no entry can
   vanish at restart. */

#define CONTACT_MAX 32                  /* lean; see DECIDE AT BUILD */

bool         contacts_load(void);       /* at startup, after identity_init() */
bool         contacts_save(void);       /* on every change, like prefs_save() */

bool         contacts_has(const char * pub_hex);
bool         contacts_trust(const char * pub_hex, const char * label);
bool         contacts_untrust(const char * pub_hex);

const char * contacts_name(const char * pub_hex);   /* NULL when absent */
int          contacts_count(void);
const char * contacts_at(int i);                    /* pubkey of the i-th contact */

/* bumped only when stored contact data actually changes (trust, untrust,
   or a label that differs from the stored one) - never on every verified
   message. The Contacts tab and the Messages list watch this. */
unsigned     contacts_version(void);

#endif
