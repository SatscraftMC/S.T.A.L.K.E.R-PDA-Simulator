#ifndef PDA_I18N_H
#define PDA_I18N_H

/* The words of the interface. The language setting on the Language page
   picks one of them; the pages' contents are still English. */

typedef enum {
    TR_TAB_MAP = 0,
    TR_TAB_MSG,
    TR_TAB_CONTACTS,
    TR_TAB_RADIO,
    TR_TAB_CAMERA,
    TR_TAB_WALLET,
    TR_TAB_OTHER,
    TR_LAYER_TASK,
    TR_LAYER_STASH,
    TR_LAYER_AREA,
    TR_LAYER_SECOND,
    TR_LAYER_IMPORT,
    TR_SEC_SETTINGS,
    TR_SEC_CONNECTIONS,
    TR_SEC_USER,
    TR_SEC_EXTRAS,
    TR_SUB_GENERAL,
    TR_SUB_ADVANCED,
    TR_SUB_LANGUAGE,
    TR_SUB_THEMES,
    TR_SUB_INFO,
    TR_SUB_BITCHAT,
    TR_SUB_FIPS,
    TR_SUB_WIFI,
    TR_SUB_GPS,
    TR_SUB_SYSTEM,
    TR_SEND,
    TR_SAVE,
    TR_SAVED,
    TR_TYPE_MESSAGE,
    TR_COUNT
} tr_id_t;

#define TR_LANG_EN 0
#define TR_LANG_PL 1
#define TR_LANG_COUNT 2

const char * tr(tr_id_t id);
void tr_set_lang(int lang);
int tr_lang(void);
void tr_load(void);                /* from the settings file */
const char * tr_lang_name(int lang);

#endif
