#ifndef PDA_PALETTE_H
#define PDA_PALETTE_H

#include <stdint.h>

/* The palette is a runtime thing: the Themes page can change the scheme and
   the accent colour, so the colours are looked up instead of compiled in.
   The names are the same as they always were, so the code that uses them
   does not have to know about it. */

typedef enum {
    PDA_C_BG = 0,
    PDA_C_TEXT,
    PDA_C_TITLE,
    PDA_C_TAB_TEXT,
    PDA_C_TAB_FILL,
    PDA_C_TAB_STROKE,
    PDA_C_TAB_ACTIVE_STROKE,
    PDA_C_TAB_ACTIVE_TEXT,
    PDA_C_TOP_LINE,
    PDA_C_BASELINE,
    PDA_C_TAG_FILL,
    PDA_C_TAG_STROKE,
    PDA_C_TAG_TEXT,
    PDA_C_SUB_FILL,
    PDA_C_SUB_EDGE,
    PDA_C_SUB_DISC,
    PDA_C_SUB_DOT_ON,
    PDA_C_SUB_DOT_OFF,
    PDA_C_SUB_TEXT_ON,
    PDA_C_CLOCK,
    PDA_C_BATTERY,
    PDA_C_BATTERY_FILL,
    PDA_C_ZOOM_BG,
    PDA_C_ZOOM_BORDER,
    PDA_C_MAP_LABEL_TOWN,
    PDA_C_MAP_LABEL_REG,
    PDA_C_MAP_LABEL_MAJOR,
    PDA_C_MAP_LABEL_SHADOW,
    PDA_C_MAP_BG,
    PDA_C_MAP_AREA,
    PDA_C_MAP_STASH,
    PDA_C_MAP_IMPORTANT,
    PDA_C_MAP_ME,
    PDA_C_CHAT_BG,
    PDA_C_CHAT_STRIP,
    PDA_C_CHAT_EDGE,
    PDA_C_CHAT_INPUT,
    PDA_C_MSG_TEXT,
    PDA_C_MSG_TIME,
    PDA_C_MSG_ME,
    PDA_C_MSG_THEM,
    PDA_C_MSG_SYS,
    PDA_C_MSG_ACCENT,
    PDA_C_CARRIER_UP,
    PDA_C_CARRIER_DOWN,
    PDA_C_CARRIER_WAIT,
    PDA_C_FRAME,
    PDA_C_FRAME_INNER,
    PDA_C_RULE,
    PDA_C_SEL_BAR,
    PDA_C_SEL_TEXT,
    PDA_C_HEAD_A,
    PDA_C_HEAD_B,
    PDA_C_PAGE_BG,
    PDA_C_PAGE_EDGE,
    PDA_C_SCROLL,
    PDA_C_COUNT
} pda_col_id_t;

/* the colour of that entry, in the current scheme */
uint32_t pda_col(pda_col_id_t id);

/* the colour schemes */
#define PDA_SCHEME_WASTELAND  0
#define PDA_SCHEME_ASH        1
#define PDA_SCHEME_RUST       2
#define PDA_SCHEME_COUNT      3

/* the accent colours */
#define PDA_ACCENT_AMBER      0
#define PDA_ACCENT_GREEN      1
#define PDA_ACCENT_BLUE       2
#define PDA_ACCENT_COUNT      3

void pda_theme_set(int scheme, int accent);
int pda_theme_scheme(void);
int pda_theme_accent(void);
void pda_theme_load(void);              /* from the settings file */
const char * pda_scheme_name(int scheme);
const char * pda_accent_name(int accent);

/* the names, unchanged - they are lookups now */
#define PDA_BG                  pda_col(PDA_C_BG)
#define PDA_TEXT                pda_col(PDA_C_TEXT)
#define PDA_TITLE               pda_col(PDA_C_TITLE)
#define PDA_TAB_TEXT            pda_col(PDA_C_TAB_TEXT)
#define PDA_TAB_FILL            pda_col(PDA_C_TAB_FILL)
#define PDA_TAB_STROKE          pda_col(PDA_C_TAB_STROKE)
#define PDA_TAB_ACTIVE_STROKE   pda_col(PDA_C_TAB_ACTIVE_STROKE)
#define PDA_TAB_ACTIVE_TEXT     pda_col(PDA_C_TAB_ACTIVE_TEXT)
#define PDA_TOP_LINE            pda_col(PDA_C_TOP_LINE)
#define PDA_BASELINE            pda_col(PDA_C_BASELINE)
#define PDA_TAG_FILL            pda_col(PDA_C_TAG_FILL)
#define PDA_TAG_STROKE          pda_col(PDA_C_TAG_STROKE)
#define PDA_TAG_TEXT            pda_col(PDA_C_TAG_TEXT)
#define PDA_SUB_FILL            pda_col(PDA_C_SUB_FILL)
#define PDA_SUB_EDGE            pda_col(PDA_C_SUB_EDGE)
#define PDA_SUB_DISC            pda_col(PDA_C_SUB_DISC)
#define PDA_SUB_DOT_ON          pda_col(PDA_C_SUB_DOT_ON)
#define PDA_SUB_DOT_OFF         pda_col(PDA_C_SUB_DOT_OFF)
#define PDA_SUB_TEXT_ON         pda_col(PDA_C_SUB_TEXT_ON)
#define PDA_CLOCK               pda_col(PDA_C_CLOCK)
#define PDA_BATTERY             pda_col(PDA_C_BATTERY)
#define PDA_BATTERY_FILL        pda_col(PDA_C_BATTERY_FILL)
#define PDA_ZOOM_BG             pda_col(PDA_C_ZOOM_BG)
#define PDA_ZOOM_BORDER         pda_col(PDA_C_ZOOM_BORDER)

#define PDA_MAP_LABEL_TOWN      pda_col(PDA_C_MAP_LABEL_TOWN)
#define PDA_MAP_LABEL_REG       pda_col(PDA_C_MAP_LABEL_REG)
#define PDA_MAP_LABEL_MAJOR     pda_col(PDA_C_MAP_LABEL_MAJOR)
#define PDA_MAP_LABEL_SHADOW    pda_col(PDA_C_MAP_LABEL_SHADOW)

#define PDA_MAP_BG              pda_col(PDA_C_MAP_BG)
#define PDA_MAP_AREA            pda_col(PDA_C_MAP_AREA)
#define PDA_MAP_STASH           pda_col(PDA_C_MAP_STASH)
#define PDA_MAP_IMPORTANT       pda_col(PDA_C_MAP_IMPORTANT)
#define PDA_MAP_ME              pda_col(PDA_C_MAP_ME)

#define PDA_CHAT_BG             pda_col(PDA_C_CHAT_BG)
#define PDA_CHAT_STRIP          pda_col(PDA_C_CHAT_STRIP)
#define PDA_CHAT_EDGE           pda_col(PDA_C_CHAT_EDGE)
#define PDA_CHAT_INPUT          pda_col(PDA_C_CHAT_INPUT)
#define PDA_MSG_TEXT            pda_col(PDA_C_MSG_TEXT)
#define PDA_MSG_TIME            pda_col(PDA_C_MSG_TIME)
#define PDA_MSG_ME              pda_col(PDA_C_MSG_ME)
#define PDA_MSG_THEM            pda_col(PDA_C_MSG_THEM)
#define PDA_MSG_SYS             pda_col(PDA_C_MSG_SYS)
#define PDA_MSG_ACCENT          pda_col(PDA_C_MSG_ACCENT)
#define PDA_CARRIER_UP          pda_col(PDA_C_CARRIER_UP)
#define PDA_CARRIER_DOWN        pda_col(PDA_C_CARRIER_DOWN)
#define PDA_CARRIER_WAIT        pda_col(PDA_C_CARRIER_WAIT)

#define PDA_FRAME               pda_col(PDA_C_FRAME)
#define PDA_FRAME_INNER         pda_col(PDA_C_FRAME_INNER)
#define PDA_RULE                pda_col(PDA_C_RULE)
#define PDA_SEL_BAR             pda_col(PDA_C_SEL_BAR)
#define PDA_SEL_TEXT            pda_col(PDA_C_SEL_TEXT)
#define PDA_HEAD_A              pda_col(PDA_C_HEAD_A)
#define PDA_HEAD_B              pda_col(PDA_C_HEAD_B)
#define PDA_PAGE_BG             pda_col(PDA_C_PAGE_BG)
#define PDA_PAGE_EDGE           pda_col(PDA_C_PAGE_EDGE)
#define PDA_SCROLL              pda_col(PDA_C_SCROLL)

#endif
