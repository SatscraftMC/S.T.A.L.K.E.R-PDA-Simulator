#include "ui/palette.h"
#include "ui/prefs.h"

#include <string.h>

/* The colour schemes.
 *
 * The first one is the wasteland palette the app started with; the other
 * two are the same shapes in cold grey and in warm rust. The accent colour
 * is layered on top of whichever scheme is in use. */

typedef struct {
    uint32_t c[PDA_C_COUNT];
} scheme_t;

static const scheme_t s_wasteland = {
    {   /* PDA_C_BG */                0x1C1E19,
        /* PDA_C_TEXT */              0x90928B,
        /* PDA_C_TITLE */             0x5C5B55,
        /* PDA_C_TAB_TEXT */          0x75766F,
        /* PDA_C_TAB_FILL */          0x1C1E19,
        /* PDA_C_TAB_STROKE */        0x3F423B,
        /* PDA_C_TAB_ACTIVE_STROKE */ 0x62655E,
        /* PDA_C_TAB_ACTIVE_TEXT */   0x90928B,
        /* PDA_C_TOP_LINE */          0x33362F,
        /* PDA_C_BASELINE */          0x3A3E36,
        /* PDA_C_TAG_FILL */          0x151711,
        /* PDA_C_TAG_STROKE */        0x32312C,
        /* PDA_C_TAG_TEXT */          0x767470,
        /* PDA_C_SUB_FILL */          0x212120,
        /* PDA_C_SUB_EDGE */          0x373836,
        /* PDA_C_SUB_DISC */          0x1C1F19,
        /* PDA_C_SUB_DOT_ON */        0x95948F,
        /* PDA_C_SUB_DOT_OFF */       0x4A4C46,
        /* PDA_C_SUB_TEXT_ON */       0xA0A19C,
        /* PDA_C_CLOCK */             0x7E7F78,
        /* PDA_C_BATTERY */           0x525349,
        /* PDA_C_BATTERY_FILL */      0x3E3D38,
        /* PDA_C_ZOOM_BG */           0x242821,
        /* PDA_C_ZOOM_BORDER */       0x3F423B,
        /* PDA_C_MAP_LABEL_TOWN */    0xA8AA9E,
        /* PDA_C_MAP_LABEL_REG */     0x8A8C82,
        /* PDA_C_MAP_LABEL_MAJOR */   0xB4B8AA,
        /* PDA_C_MAP_LABEL_SHADOW */  0x0E100C,
        /* PDA_C_MAP_BG */            0x343A2C,
        /* PDA_C_MAP_AREA */          0xFFFFFF,
        /* PDA_C_MAP_STASH */         0xFFD32A,
        /* PDA_C_MAP_IMPORTANT */     0xF39C12,
        /* PDA_C_MAP_ME */            0x3FA9F5,
        /* PDA_C_CHAT_BG */           0x191B16,
        /* PDA_C_CHAT_STRIP */        0x12130F,
        /* PDA_C_CHAT_EDGE */         0x35382F,
        /* PDA_C_CHAT_INPUT */        0x20221C,
        /* PDA_C_MSG_TEXT */          0x9B9D94,
        /* PDA_C_MSG_TIME */          0x55574E,
        /* PDA_C_MSG_ME */            0x4FA8D8,
        /* PDA_C_MSG_THEM */          0xC9A94E,
        /* PDA_C_MSG_SYS */           0x6C6E64,
        /* PDA_C_MSG_ACCENT */        0xC9A227,
        /* PDA_C_CARRIER_UP */        0x7C9E52,
        /* PDA_C_CARRIER_DOWN */      0xB05545,
        /* PDA_C_CARRIER_WAIT */      0x8B8D85,
        /* PDA_C_FRAME */             0x3E4239,
        /* PDA_C_FRAME_INNER */       0x2A2D26,
        /* PDA_C_RULE */              0x242720,
        /* PDA_C_SEL_BAR */           0x3B3E37,
        /* PDA_C_SEL_TEXT */          0xC0C2B6,
        /* PDA_C_HEAD_A */            0x7FA24F,
        /* PDA_C_HEAD_B */            0x5E8CB0,
        /* PDA_C_PAGE_BG */           0x0E0F0B,
        /* PDA_C_PAGE_EDGE */         0x2A2D26,
        /* PDA_C_SCROLL */            0x4A4D45 }
};

static const scheme_t s_ash = {
    {   /* PDA_C_BG */                0x191C20,
        /* PDA_C_TEXT */              0x8E949B,
        /* PDA_C_TITLE */             0x585D64,
        /* PDA_C_TAB_TEXT */          0x71777E,
        /* PDA_C_TAB_FILL */          0x191C20,
        /* PDA_C_TAB_STROKE */        0x3B4148,
        /* PDA_C_TAB_ACTIVE_STROKE */ 0x5D656E,
        /* PDA_C_TAB_ACTIVE_TEXT */   0x8E949B,
        /* PDA_C_TOP_LINE */          0x30353B,
        /* PDA_C_BASELINE */          0x373D44,
        /* PDA_C_TAG_FILL */          0x14171A,
        /* PDA_C_TAG_STROKE */        0x2F3439,
        /* PDA_C_TAG_TEXT */          0x737A80,
        /* PDA_C_SUB_FILL */          0x1E2126,
        /* PDA_C_SUB_EDGE */          0x353A41,
        /* PDA_C_SUB_DISC */          0x191D22,
        /* PDA_C_SUB_DOT_ON */        0x919AA1,
        /* PDA_C_SUB_DOT_OFF */       0x474D55,
        /* PDA_C_SUB_TEXT_ON */       0x9CA4AC,
        /* PDA_C_CLOCK */             0x7B8188,
        /* PDA_C_BATTERY */           0x4E555D,
        /* PDA_C_BATTERY_FILL */      0x3A4048,
        /* PDA_C_ZOOM_BG */           0x22262B,
        /* PDA_C_ZOOM_BORDER */       0x3B4148,
        /* PDA_C_MAP_LABEL_TOWN */    0xA3ABB3,
        /* PDA_C_MAP_LABEL_REG */     0x868E96,
        /* PDA_C_MAP_LABEL_MAJOR */   0xB0B8C0,
        /* PDA_C_MAP_LABEL_SHADOW */  0x0D1013,
        /* PDA_C_MAP_BG */            0x2C3238,
        /* PDA_C_MAP_AREA */          0xFFFFFF,
        /* PDA_C_MAP_STASH */         0xFFD32A,
        /* PDA_C_MAP_IMPORTANT */     0xF39C12,
        /* PDA_C_MAP_ME */            0x3FA9F5,
        /* PDA_C_CHAT_BG */           0x161A1E,
        /* PDA_C_CHAT_STRIP */        0x101315,
        /* PDA_C_CHAT_EDGE */         0x333940,
        /* PDA_C_CHAT_INPUT */        0x1E2226,
        /* PDA_C_MSG_TEXT */          0x98A0A8,
        /* PDA_C_MSG_TIME */          0x535A62,
        /* PDA_C_MSG_ME */            0x4FA8D8,
        /* PDA_C_MSG_THEM */          0xC9A94E,
        /* PDA_C_MSG_SYS */           0x6A727A,
        /* PDA_C_MSG_ACCENT */        0xC9A227,
        /* PDA_C_CARRIER_UP */        0x7C9E52,
        /* PDA_C_CARRIER_DOWN */      0xB05545,
        /* PDA_C_CARRIER_WAIT */      0x8B8D85,
        /* PDA_C_FRAME */             0x3A4148,
        /* PDA_C_FRAME_INNER */       0x272D33,
        /* PDA_C_RULE */              0x232930,
        /* PDA_C_SEL_BAR */           0x373E45,
        /* PDA_C_SEL_TEXT */          0xBEC5CC,
        /* PDA_C_HEAD_A */            0x7FA24F,
        /* PDA_C_HEAD_B */            0x5E8CB0,
        /* PDA_C_PAGE_BG */           0x0D1013,
        /* PDA_C_PAGE_EDGE */         0x272D33,
        /* PDA_C_SCROLL */            0x474E56 }
};

static const scheme_t s_rust = {
    {   /* PDA_C_BG */                0x1F1B17,
        /* PDA_C_TEXT */              0x968C80,
        /* PDA_C_TITLE */             0x5E574E,
        /* PDA_C_TAB_TEXT */          0x7A7267,
        /* PDA_C_TAB_FILL */          0x1F1B17,
        /* PDA_C_TAB_STROKE */        0x433C34,
        /* PDA_C_TAB_ACTIVE_STROKE */ 0x675C50,
        /* PDA_C_TAB_ACTIVE_TEXT */   0x968C80,
        /* PDA_C_TOP_LINE */          0x362F28,
        /* PDA_C_BASELINE */          0x3E362D,
        /* PDA_C_TAG_FILL */          0x181410,
        /* PDA_C_TAG_STROKE */        0x362F29,
        /* PDA_C_TAG_TEXT */          0x7B7367,
        /* PDA_C_SUB_FILL */          0x24201B,
        /* PDA_C_SUB_EDGE */          0x3A342D,
        /* PDA_C_SUB_DISC */          0x1F1C17,
        /* PDA_C_SUB_DOT_ON */        0x9A9186,
        /* PDA_C_SUB_DOT_OFF */       0x4E4740,
        /* PDA_C_SUB_TEXT_ON */       0xA59B8E,
        /* PDA_C_CLOCK */             0x847B6F,
        /* PDA_C_BATTERY */           0x574F45,
        /* PDA_C_BATTERY_FILL */      0x413A32,
        /* PDA_C_ZOOM_BG */           0x262119,
        /* PDA_C_ZOOM_BORDER */       0x433C34,
        /* PDA_C_MAP_LABEL_TOWN */    0xAFA497,
        /* PDA_C_MAP_LABEL_REG */     0x91877B,
        /* PDA_C_MAP_LABEL_MAJOR */   0xBAB0A2,
        /* PDA_C_MAP_LABEL_SHADOW */  0x100D0A,
        /* PDA_C_MAP_BG */            0x39342A,
        /* PDA_C_MAP_AREA */          0xFFFFFF,
        /* PDA_C_MAP_STASH */         0xFFD32A,
        /* PDA_C_MAP_IMPORTANT */     0xF39C12,
        /* PDA_C_MAP_ME */            0x3FA9F5,
        /* PDA_C_CHAT_BG */           0x1C1815,
        /* PDA_C_CHAT_STRIP */        0x14110F,
        /* PDA_C_CHAT_EDGE */         0x3A342C,
        /* PDA_C_CHAT_INPUT */        0x241F1A,
        /* PDA_C_MSG_TEXT */          0xA09689,
        /* PDA_C_MSG_TIME */          0x5A5249,
        /* PDA_C_MSG_ME */            0x4FA8D8,
        /* PDA_C_MSG_THEM */          0xC9A94E,
        /* PDA_C_MSG_SYS */           0x726859,
        /* PDA_C_MSG_ACCENT */        0xC9A227,
        /* PDA_C_CARRIER_UP */        0x7C9E52,
        /* PDA_C_CARRIER_DOWN */      0xB05545,
        /* PDA_C_CARRIER_WAIT */      0x8B8D85,
        /* PDA_C_FRAME */             0x433C33,
        /* PDA_C_FRAME_INNER */       0x2E2823,
        /* PDA_C_RULE */              0x28231D,
        /* PDA_C_SEL_BAR */           0x3F382F,
        /* PDA_C_SEL_TEXT */          0xC6BCAD,
        /* PDA_C_HEAD_A */            0x7FA24F,
        /* PDA_C_HEAD_B */            0x5E8CB0,
        /* PDA_C_PAGE_BG */           0x110E0C,
        /* PDA_C_PAGE_EDGE */         0x2E2823,
        /* PDA_C_SCROLL */            0x4E463D }
};

static const scheme_t * const s_schemes[PDA_SCHEME_COUNT] = {
    &s_wasteland, &s_ash, &s_rust
};

static const uint32_t s_accents[PDA_ACCENT_COUNT] = {
    0xC9A227,     /* amber */
    0x8FBF4A,     /* green */
    0x4FA8D8      /* blue */
};

static int s_scheme = PDA_SCHEME_WASTELAND;
static int s_accent = PDA_ACCENT_AMBER;

uint32_t pda_col(pda_col_id_t id)
{
    if(id < 0 || id >= PDA_C_COUNT) return 0xFF000000;
    if(id == PDA_C_MSG_ACCENT) return s_accents[s_accent];
    return s_schemes[s_scheme]->c[id];
}

void pda_theme_set(int scheme, int accent)
{
    if(scheme < 0 || scheme >= PDA_SCHEME_COUNT) scheme = PDA_SCHEME_WASTELAND;
    if(accent < 0 || accent >= PDA_ACCENT_COUNT) accent = PDA_ACCENT_AMBER;
    s_scheme = scheme;
    s_accent = accent;
}

int pda_theme_scheme(void)
{
    return s_scheme;
}

int pda_theme_accent(void)
{
    return s_accent;
}

void pda_theme_load(void)
{
    const char * s = prefs_str("scheme", "wasteland");
    int scheme = PDA_SCHEME_WASTELAND;
    if(!strcmp(s, "ash")) scheme = PDA_SCHEME_ASH;
    else if(!strcmp(s, "rust")) scheme = PDA_SCHEME_RUST;

    const char * a = prefs_str("accent", "amber");
    int accent = PDA_ACCENT_AMBER;
    if(!strcmp(a, "green")) accent = PDA_ACCENT_GREEN;
    else if(!strcmp(a, "blue")) accent = PDA_ACCENT_BLUE;

    pda_theme_set(scheme, accent);
}

const char * pda_scheme_name(int scheme)
{
    switch(scheme) {
        case PDA_SCHEME_ASH: return "Ash";
        case PDA_SCHEME_RUST: return "Rust";
        default: return "Wasteland";
    }
}

const char * pda_accent_name(int accent)
{
    switch(accent) {
        case PDA_ACCENT_GREEN: return "Green";
        case PDA_ACCENT_BLUE: return "Blue";
        default: return "Amber";
    }
}
