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

static const char * const s_de[TR_COUNT] = {
    [TR_TAB_MAP]        = "Karte",
    [TR_TAB_MSG]        = "Nachrichten",
    [TR_TAB_CONTACTS]   = "Kontakte",
    [TR_TAB_RADIO]      = "Radio",
    [TR_TAB_CAMERA]     = "Kamera",
    [TR_TAB_WALLET]     = "Wallet",
    [TR_TAB_OTHER]      = "Sonstiges",
    [TR_LAYER_TASK]     = "Aufgabenliste",
    [TR_LAYER_STASH]    = "Lager",
    [TR_LAYER_AREA]     = "Gebiete",
    [TR_LAYER_SECOND]   = "Nebenziele",
    [TR_LAYER_IMPORT]   = "Wichtige Objekte",
    [TR_SEC_SETTINGS]   = "Einstellungen",
    [TR_SEC_CONNECTIONS]= "Verbindungen",
    [TR_SEC_USER]       = "Benutzer",
    [TR_SEC_EXTRAS]     = "Extras",
    [TR_SUB_GENERAL]    = "Allgemein",
    [TR_SUB_ADVANCED]   = "Erweitert",
    [TR_SUB_LANGUAGE]   = "Sprache",
    [TR_SUB_THEMES]     = "Designs",
    [TR_SUB_INFO]       = "Info",
    [TR_SUB_BITCHAT]    = "Bitchat",
    [TR_SUB_FIPS]       = "F.I.P.S",
    [TR_SUB_WIFI]       = "WLAN",
    [TR_SUB_GPS]        = "GPS",
    [TR_SUB_SYSTEM]     = "System",
    [TR_SEND]           = "SENDEN",
    [TR_SAVE]           = "SPEICHERN",
    [TR_SAVED]          = "GESPEICHERT",
    [TR_TYPE_MESSAGE]   = "Nachricht schreiben",
};

static int s_lang = TR_LANG_EN;

const char * tr(tr_id_t id)
{
    if(id < 0 || id >= TR_COUNT) return "";
    const char * const * tab = (s_lang == TR_LANG_DE) ? s_de : s_en;
    const char * s = tab[id];
    return s ? s : s_en[id];
}

void tr_set_lang(int lang)
{
    s_lang = (lang == TR_LANG_DE) ? TR_LANG_DE : TR_LANG_EN;
}

int tr_lang(void)
{
    return s_lang;
}

void tr_load(void)
{
    const char * l = prefs_str("lang", "en");
    tr_set_lang(!strcmp(l, "de") ? TR_LANG_DE : TR_LANG_EN);
}

const char * tr_lang_name(int lang)
{
    return (lang == TR_LANG_DE) ? "Deutsch" : "English";
}
