/*******************************************************************************
 * Size: 14 px
 * Bpp: 1
 * Opts: --font /home/aj/pda-project/app/build/fonts-src/ShareTechMono-Regular.ttf --size 14 --bpp 1 --format lvgl --lv-include lvgl.h --no-compress -r 0x20-0x7F --lv-font-name pda_font_mono_14 -o /home/aj/pda-project/app/src/assets/fonts/pda_font_mono_14.c
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl.h"
#endif

#ifndef PDA_FONT_MONO_14
#define PDA_FONT_MONO_14 1
#endif

#if PDA_FONT_MONO_14

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0021 "!" */
    0xfe, 0x40,

    /* U+0022 "\"" */
    0xb6, 0x80,

    /* U+0023 "#" */
    0x52, 0x95, 0xf5, 0x2b, 0xea, 0x52, 0x80,

    /* U+0024 "$" */
    0x21, 0x1f, 0x8, 0x41, 0xc1, 0x8, 0x7c, 0x42,
    0x0,

    /* U+0025 "%" */
    0x78, 0x48, 0x48, 0x78, 0xf, 0xf8, 0xe, 0xa,
    0xa, 0xe,

    /* U+0026 "&" */
    0x72, 0x8, 0x20, 0x7e, 0x28, 0xa2, 0x89, 0xe0,

    /* U+0027 "'" */
    0xe0,

    /* U+0028 "(" */
    0x5a, 0x49, 0x24, 0x92, 0x64,

    /* U+0029 ")" */
    0x4c, 0x92, 0x49, 0x24, 0xb4,

    /* U+002A "*" */
    0x22, 0x8f, 0x9c, 0x58,

    /* U+002B "+" */
    0x21, 0x3e, 0x42, 0x0,

    /* U+002C "," */
    0x54,

    /* U+002D "-" */
    0xf8,

    /* U+002E "." */
    0x80,

    /* U+002F "/" */
    0xc, 0x20, 0x86, 0x10, 0x42, 0x8, 0x61, 0x4,
    0x20, 0x80,

    /* U+0030 "0" */
    0x74, 0x63, 0x3b, 0xf7, 0x31, 0x8b, 0x80,

    /* U+0031 "1" */
    0xe5, 0x8, 0x42, 0x10, 0x84, 0x27, 0xc0,

    /* U+0032 "2" */
    0xe0, 0x84, 0x21, 0x11, 0x88, 0xc7, 0xc0,

    /* U+0033 "3" */
    0xe1, 0x11, 0xe1, 0x11, 0x1e,

    /* U+0034 "4" */
    0x62, 0x10, 0x85, 0x6a, 0x5f, 0x10, 0x80,

    /* U+0035 "5" */
    0xf8, 0x88, 0xe1, 0x11, 0x1e,

    /* U+0036 "6" */
    0x74, 0x21, 0xf, 0x46, 0x31, 0x8b, 0x80,

    /* U+0037 "7" */
    0xf8, 0x46, 0x21, 0x18, 0x84, 0x62, 0x0,

    /* U+0038 "8" */
    0x74, 0x63, 0x17, 0x46, 0x31, 0x8b, 0x80,

    /* U+0039 "9" */
    0x74, 0x63, 0x18, 0xbc, 0x21, 0xb, 0x80,

    /* U+003A ":" */
    0x82,

    /* U+003B ";" */
    0x40, 0x15,

    /* U+003C "<" */
    0x4, 0xee, 0x30, 0x70, 0x70, 0x40,

    /* U+003D "=" */
    0xf8, 0x1, 0xf0,

    /* U+003E ">" */
    0x83, 0x81, 0x83, 0x3b, 0x0, 0x0,

    /* U+003F "?" */
    0xe1, 0x11, 0x24, 0x40, 0x4,

    /* U+0040 "@" */
    0x7d, 0x6, 0xed, 0x5a, 0xb5, 0x6a, 0xdd, 0x87,
    0xf0,

    /* U+0041 "A" */
    0x30, 0xc5, 0x14, 0x49, 0x2f, 0xa2, 0x8e, 0x10,

    /* U+0042 "B" */
    0xf4, 0x63, 0x1f, 0x46, 0x31, 0x8f, 0x80,

    /* U+0043 "C" */
    0x7c, 0x21, 0x8, 0x42, 0x10, 0x83, 0xc0,

    /* U+0044 "D" */
    0xf4, 0x63, 0x18, 0xc6, 0x31, 0x8f, 0x80,

    /* U+0045 "E" */
    0xf8, 0x88, 0xf8, 0x88, 0x8f,

    /* U+0046 "F" */
    0xf8, 0x88, 0x8f, 0x88, 0x88,

    /* U+0047 "G" */
    0x7c, 0x21, 0x8, 0x4e, 0x31, 0x8b, 0xc0,

    /* U+0048 "H" */
    0x8c, 0x63, 0x1f, 0xc6, 0x31, 0x8c, 0x40,

    /* U+0049 "I" */
    0xf4, 0x44, 0x44, 0x44, 0x4f,

    /* U+004A "J" */
    0x71, 0x11, 0x11, 0x11, 0x1e,

    /* U+004B "K" */
    0x9c, 0xa9, 0xcc, 0x72, 0x92, 0x94, 0xc0,

    /* U+004C "L" */
    0x88, 0x88, 0x88, 0x88, 0x8f,

    /* U+004D "M" */
    0xcf, 0x3c, 0xed, 0xb6, 0xdb, 0x61, 0x86, 0x10,

    /* U+004E "N" */
    0xce, 0x73, 0x5a, 0xd6, 0xb3, 0x9c, 0xc0,

    /* U+004F "O" */
    0x74, 0x63, 0x18, 0xc6, 0x31, 0x8b, 0x80,

    /* U+0050 "P" */
    0xf4, 0x63, 0x18, 0xfa, 0x10, 0x84, 0x0,

    /* U+0051 "Q" */
    0x72, 0x28, 0xa2, 0x8a, 0x28, 0xa2, 0x89, 0xe0,
    0x0,

    /* U+0052 "R" */
    0xf2, 0x28, 0xa2, 0x8b, 0xc9, 0x26, 0x8a, 0x20,

    /* U+0053 "S" */
    0x7c, 0x21, 0x7, 0x4, 0x21, 0xf, 0x80,

    /* U+0054 "T" */
    0xfc, 0x82, 0x8, 0x20, 0x82, 0x8, 0x20, 0x80,

    /* U+0055 "U" */
    0x8c, 0x63, 0x18, 0xc6, 0x31, 0x8b, 0x80,

    /* U+0056 "V" */
    0x8e, 0x28, 0xb2, 0x49, 0x65, 0x14, 0x30, 0xc0,

    /* U+0057 "W" */
    0x86, 0x18, 0x61, 0xb6, 0xdb, 0x7f, 0xcf, 0x30,

    /* U+0058 "X" */
    0xc9, 0x25, 0x8c, 0x30, 0xc7, 0x16, 0xca, 0x30,

    /* U+0059 "Y" */
    0x44, 0x88, 0xa1, 0x41, 0x2, 0x4, 0x8, 0x10,
    0x20,

    /* U+005A "Z" */
    0xf8, 0xc4, 0x62, 0x11, 0x8, 0x87, 0xc0,

    /* U+005B "[" */
    0xf2, 0x49, 0x24, 0x92, 0x4e,

    /* U+005C "\\" */
    0x82, 0x4, 0x10, 0x60, 0x82, 0x4, 0x10, 0x60,
    0x82, 0xc,

    /* U+005D "]" */
    0xe4, 0x92, 0x49, 0x24, 0x9e,

    /* U+005E "^" */
    0x31, 0xc5, 0x92, 0x88,

    /* U+005F "_" */
    0xfe,

    /* U+0060 "`" */
    0x44,

    /* U+0061 "a" */
    0xf0, 0x20, 0x9e, 0x8a, 0x2f, 0xc0,

    /* U+0062 "b" */
    0x84, 0x21, 0xe8, 0xc6, 0x31, 0x8f, 0x80,

    /* U+0063 "c" */
    0x78, 0x88, 0x88, 0x70,

    /* U+0064 "d" */
    0x8, 0x42, 0xf8, 0xc6, 0x31, 0x8b, 0xc0,

    /* U+0065 "e" */
    0x74, 0x63, 0xf8, 0x41, 0xe0,

    /* U+0066 "f" */
    0x7a, 0x11, 0xf4, 0x21, 0x8, 0x47, 0x80,

    /* U+0067 "g" */
    0xfc, 0x63, 0xe4, 0x3c, 0x21, 0xf8,

    /* U+0068 "h" */
    0x84, 0x21, 0xe8, 0xc6, 0x31, 0x8c, 0x40,

    /* U+0069 "i" */
    0x20, 0x1, 0xc2, 0x10, 0x84, 0x21, 0xc0,

    /* U+006A "j" */
    0x20, 0x72, 0x49, 0x24, 0xf0,

    /* U+006B "k" */
    0x84, 0x21, 0x2a, 0x73, 0x94, 0x94, 0xc0,

    /* U+006C "l" */
    0xe1, 0x8, 0x42, 0x10, 0x84, 0x21, 0xc0,

    /* U+006D "m" */
    0xff, 0x26, 0x4c, 0x99, 0x32, 0x64, 0x80,

    /* U+006E "n" */
    0xf4, 0x63, 0x18, 0xc6, 0x20,

    /* U+006F "o" */
    0x74, 0x63, 0x18, 0xc5, 0xc0,

    /* U+0070 "p" */
    0xf4, 0x63, 0x18, 0xc7, 0xd0, 0x80,

    /* U+0071 "q" */
    0x7c, 0x63, 0x18, 0xc5, 0xe1, 0x8,

    /* U+0072 "r" */
    0xfa, 0x10, 0x84, 0x23, 0xc0,

    /* U+0073 "s" */
    0xf8, 0x8f, 0x11, 0xf0,

    /* U+0074 "t" */
    0x42, 0x3e, 0x84, 0x21, 0x8, 0x78,

    /* U+0075 "u" */
    0x8c, 0x63, 0x18, 0xc5, 0xe0,

    /* U+0076 "v" */
    0x8f, 0x24, 0x96, 0x51, 0xc3, 0x0,

    /* U+0077 "w" */
    0x42, 0xa5, 0x6a, 0xe5, 0xcd, 0x8b, 0x0,

    /* U+0078 "x" */
    0xc9, 0x63, 0xc, 0x71, 0x6c, 0x80,

    /* U+0079 "y" */
    0x8f, 0x24, 0x92, 0x70, 0xc3, 0xc, 0x60,

    /* U+007A "z" */
    0xf3, 0x26, 0x48, 0xf0,

    /* U+007B "{" */
    0x69, 0x24, 0xa2, 0x49, 0x26,

    /* U+007C "|" */
    0xff, 0xf8,

    /* U+007D "}" */
    0xc9, 0x24, 0x8a, 0x49, 0x2c,

    /* U+007E "~" */
    0x6c, 0xc0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 121, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 121, .box_w = 1, .box_h = 10, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 3, .adv_w = 121, .box_w = 3, .box_h = 3, .ofs_x = 2, .ofs_y = 7},
    {.bitmap_index = 5, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 12, .adv_w = 121, .box_w = 5, .box_h = 13, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 21, .adv_w = 121, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 31, .adv_w = 121, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 39, .adv_w = 121, .box_w = 1, .box_h = 3, .ofs_x = 3, .ofs_y = 7},
    {.bitmap_index = 40, .adv_w = 121, .box_w = 3, .box_h = 13, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 45, .adv_w = 121, .box_w = 3, .box_h = 13, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 50, .adv_w = 121, .box_w = 6, .box_h = 5, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 54, .adv_w = 121, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 58, .adv_w = 121, .box_w = 2, .box_h = 3, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 59, .adv_w = 121, .box_w = 5, .box_h = 1, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 60, .adv_w = 121, .box_w = 1, .box_h = 1, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 61, .adv_w = 121, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 71, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 78, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 85, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 92, .adv_w = 121, .box_w = 4, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 97, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 104, .adv_w = 121, .box_w = 4, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 109, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 116, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 123, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 130, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 137, .adv_w = 121, .box_w = 1, .box_h = 7, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 138, .adv_w = 121, .box_w = 2, .box_h = 8, .ofs_x = 2, .ofs_y = -1},
    {.bitmap_index = 140, .adv_w = 121, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 146, .adv_w = 121, .box_w = 5, .box_h = 4, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 149, .adv_w = 121, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 155, .adv_w = 121, .box_w = 4, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 160, .adv_w = 121, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 169, .adv_w = 121, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 177, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 184, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 191, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 198, .adv_w = 121, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 203, .adv_w = 121, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 208, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 215, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 222, .adv_w = 121, .box_w = 4, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 227, .adv_w = 121, .box_w = 4, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 232, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 239, .adv_w = 121, .box_w = 4, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 244, .adv_w = 121, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 252, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 259, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 266, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 273, .adv_w = 121, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 282, .adv_w = 121, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 290, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 297, .adv_w = 121, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 305, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 312, .adv_w = 121, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 320, .adv_w = 121, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 328, .adv_w = 121, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 336, .adv_w = 121, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 345, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 352, .adv_w = 121, .box_w = 3, .box_h = 13, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 357, .adv_w = 121, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 367, .adv_w = 121, .box_w = 3, .box_h = 13, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 372, .adv_w = 121, .box_w = 6, .box_h = 5, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 376, .adv_w = 121, .box_w = 7, .box_h = 1, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 377, .adv_w = 121, .box_w = 3, .box_h = 2, .ofs_x = 2, .ofs_y = 8},
    {.bitmap_index = 378, .adv_w = 121, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 384, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 391, .adv_w = 121, .box_w = 4, .box_h = 7, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 395, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 402, .adv_w = 121, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 407, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 414, .adv_w = 121, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 420, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 427, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 434, .adv_w = 121, .box_w = 3, .box_h = 12, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 439, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 446, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 453, .adv_w = 121, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 460, .adv_w = 121, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 465, .adv_w = 121, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 470, .adv_w = 121, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 476, .adv_w = 121, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 482, .adv_w = 121, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 487, .adv_w = 121, .box_w = 4, .box_h = 7, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 491, .adv_w = 121, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 497, .adv_w = 121, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 502, .adv_w = 121, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 508, .adv_w = 121, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 515, .adv_w = 121, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 521, .adv_w = 121, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 528, .adv_w = 121, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 532, .adv_w = 121, .box_w = 3, .box_h = 13, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 537, .adv_w = 121, .box_w = 1, .box_h = 13, .ofs_x = 3, .ofs_y = -2},
    {.bitmap_index = 539, .adv_w = 121, .box_w = 3, .box_h = 13, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 544, .adv_w = 121, .box_w = 5, .box_h = 2, .ofs_x = 1, .ofs_y = 4}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/



/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 95, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 1,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};



/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t pda_font_mono_14 = {
#else
lv_font_t pda_font_mono_14 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 13,          /*The maximum line height required by the font*/
    .base_line = 2,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -2,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if PDA_FONT_MONO_14*/

