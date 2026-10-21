#include "ui/i18n.h"
#include "ui/prefs.h"

#include <string.h>

static const char * const s_en[TR_COUNT] = {
    [TR_TAB_MAP]        = "Area map",
    [TR_TAB_MSG]        = "Messages",
    [TR_TAB_CONTACTS]   = "Contacts",
    [TR_TAB_RADIO]      = "Radio",
    [TR_TAB_CAMERA]     = "Camera",
    [TR_TAB_WALLET]     = "Wallet",
    [TR_TAB_OTHER]      = "Other",
    [TR_LAYER_TASK]     = "Task list",
    [TR_LAYER_STASH]    = "Stashes",
    [TR_LAYER_AREA]     = "Areas",
    [TR_LAYER_SECOND]   = "Secondary tasks",
    [TR_LAYER_IMPORT]   = "Important objects",
    [TR_SEC_SETTINGS]   = "Settings",
    [TR_SEC_CONNECTIONS]= "Connections",
    [TR_SEC_USER]       = "User",
    [TR_SEC_EXTRAS]     = "Extras",
    [TR_SUB_GENERAL]    = "General",
    [TR_SUB_ADVANCED]   = "Advanced",
    [TR_SUB_LANGUAGE]   = "Language",
    [TR_SUB_THEMES]     = "Themes",
    [TR_SUB_INFO]       = "Info",
    [TR_SUB_BITCHAT]    = "Bitchat",
    [TR_SUB_FIPS]       = "F.I.P.S",
    [TR_SUB_WIFI]       = "WiFi",
    [TR_SUB_GPS]        = "GPS",
    [TR_SUB_SYSTEM]     = "System",
    [TR_SEND]           = "SEND",
    [TR_SAVE]           = "SAVE",
    [TR_SAVED]          = "SAVED",
    [TR_TYPE_MESSAGE]   = "type a message",
};

static const char * const s_pl[TR_COUNT] = {
    [TR_TAB_MAP]        = "Mapa",
    [TR_TAB_MSG]        = "Wiadomo\u015bci",
    [TR_TAB_CONTACTS]   = "Kontakty",
    [TR_TAB_RADIO]      = "Radio",
    [TR_TAB_CAMERA]     = "Kamera",
    [TR_TAB_WALLET]     = "Portfel",
    [TR_TAB_OTHER]      = "Inne",
    [TR_LAYER_TASK]     = "Lista zada\u0144",
    [TR_LAYER_STASH]    = "Schowki",
    [TR_LAYER_AREA]     = "Obszary",
    [TR_LAYER_SECOND]   = "Zadania drugorz\u0119dne",
    [TR_LAYER_IMPORT]   = "Wa\u017cne obiekty",
    [TR_SEC_SETTINGS]   = "Ustawienia",
    [TR_SEC_CONNECTIONS]= "Po\u0142\u0105czenia",
    [TR_SEC_USER]       = "U\u017cytkownik",
    [TR_SEC_EXTRAS]     = "Dodatki",
    [TR_SUB_GENERAL]    = "Og\u00f3lne",
    [TR_SUB_ADVANCED]   = "Zaawansowane",
    [TR_SUB_LANGUAGE]   = "J\u0119zyk",
    [TR_SUB_THEMES]     = "Motywy",
    [TR_SUB_INFO]       = "Info",
    [TR_SUB_BITCHAT]    = "Bitchat",
    [TR_SUB_FIPS]       = "F.I.P.S",
    [TR_SUB_WIFI]       = "WiFi",
    [TR_SUB_GPS]        = "GPS",
    [TR_SUB_SYSTEM]     = "System",
    [TR_SEND]           = "WY\u015aLIJ",
    [TR_SAVE]           = "ZAPISZ",
    [TR_SAVED]          = "ZAPISANO",
    [TR_TYPE_MESSAGE]   = "napisz wiadomo\u015b\u0107",
};

static int s_lang = TR_LANG_EN;

const char * tr(tr_id_t id)
{
    if(id < 0 || id >= TR_COUNT) return "";
    const char * const * tab = (s_lang == TR_LANG_PL) ? s_pl : s_en;
    const char * s = tab[id];
    return s ? s : s_en[id];
}

void tr_set_lang(int lang)
{
    s_lang = (lang == TR_LANG_PL) ? TR_LANG_PL : TR_LANG_EN;
}

int tr_lang(void)
{
    return s_lang;
}

void tr_load(void)
{
    const char * l = prefs_str("lang", "en");
    tr_set_lang(!strcmp(l, "pl") ? TR_LANG_PL : TR_LANG_EN);
}

const char * tr_lang_name(int lang)
{
    return (lang == TR_LANG_PL) ? "Polski" : "English";
}
