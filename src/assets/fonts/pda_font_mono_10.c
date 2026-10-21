/*******************************************************************************
 * Size: 10 px
 * Bpp: 1
 * Opts: --font /home/aj/pda-project/app/build/fonts-src/ShareTechMono-Regular.ttf --size 10 --bpp 1 --format lvgl --lv-include lvgl.h --no-compress -r 0x20-0x7F --lv-font-name pda_font_mono_10 -o /home/aj/pda-project/app/src/assets/fonts/pda_font_mono_10.c
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl.h"
#endif

#ifndef PDA_FONT_MONO_10
#define PDA_FONT_MONO_10 1
#endif

#if PDA_FONT_MONO_10

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0021 "!" */
    0xfa,

    /* U+0022 "\"" */
    0xb4,

    /* U+0023 "#" */
    0x52, 0xbe, 0xaf, 0xa9, 0x40,

    /* U+0024 "$" */
    0x5e, 0x4e, 0x4f, 0x40,

    /* U+0025 "%" */
    0xe5, 0x39, 0xf3, 0x94, 0xe0,

    /* U+0026 "&" */
    0xc8, 0x7a, 0xaa, 0xe0,

    /* U+0027 "'" */
    0xc0,

    /* U+0028 "(" */
    0x6a, 0xaa, 0x90,

    /* U+0029 ")" */
    0x95, 0x55, 0x60,

    /* U+002A "*" */
    0x4f, 0x6a,

    /* U+002B "+" */
    0x21, 0x3e, 0x40,

    /* U+002C "," */
    0xc0,

    /* U+002D "-" */
    0xe0,

    /* U+002E "." */
    0x80,

    /* U+002F "/" */
    0x8, 0x84, 0x22, 0x11, 0x8, 0x40,

    /* U+0030 "0" */
    0xf9, 0xbd, 0xd9, 0xf0,

    /* U+0031 "1" */
    0xc9, 0x24, 0xb8,

    /* U+0032 "2" */
    0xe4, 0x95, 0x38,

    /* U+0033 "3" */
    0xe4, 0xe2, 0x78,

    /* U+0034 "4" */
    0x44, 0x8a, 0xaf, 0x20,

    /* U+0035 "5" */
    0xf2, 0x72, 0x78,

    /* U+0036 "6" */
    0xe8, 0xf9, 0x99, 0xe0,

    /* U+0037 "7" */
    0xe4, 0xa4, 0xa0,

    /* U+0038 "8" */
    0xf9, 0x96, 0x99, 0xf0,

    /* U+0039 "9" */
    0x79, 0x99, 0xf1, 0x70,

    /* U+003A ":" */
    0x88,

    /* U+003B ";" */
    0x8c,

    /* U+003C "<" */
    0x7, 0x86, 0x10,

    /* U+003D "=" */
    0xf0, 0xf0,

    /* U+003E ">" */
    0xe, 0x16, 0x80,

    /* U+003F "?" */
    0xe4, 0xa4, 0x10,

    /* U+0040 "@" */
    0xff, 0xf7, 0xbd, 0xff, 0xc0,

    /* U+0041 "A" */
    0x22, 0x94, 0xa7, 0x26, 0x20,

    /* U+0042 "B" */
    0xf9, 0x9e, 0x99, 0xf0,

    /* U+0043 "C" */
    0xf2, 0x49, 0x38,

    /* U+0044 "D" */
    0xf9, 0x99, 0x99, 0xf0,

    /* U+0045 "E" */
    0xf2, 0x79, 0x38,

    /* U+0046 "F" */
    0xf2, 0x79, 0x20,

    /* U+0047 "G" */
    0xf8, 0x8b, 0x99, 0xf0,

    /* U+0048 "H" */
    0x99, 0x9f, 0x99, 0x90,

    /* U+0049 "I" */
    0xe9, 0x24, 0xb8,

    /* U+004A "J" */
    0xe4, 0x92, 0x78,

    /* U+004B "K" */
    0x9a, 0xcc, 0xaa, 0x90,

    /* U+004C "L" */
    0x92, 0x49, 0x38,

    /* U+004D "M" */
    0xdf, 0xff, 0x99, 0x90,

    /* U+004E "N" */
    0xdd, 0xdd, 0xbb, 0xb0,

    /* U+004F "O" */
    0xf9, 0x99, 0x99, 0xf0,

    /* U+0050 "P" */
    0xf9, 0x9f, 0x88, 0x80,

    /* U+0051 "Q" */
    0xf4, 0xa5, 0x29, 0x4b, 0xc0,

    /* U+0052 "R" */
    0xf4, 0xa5, 0xca, 0x4a, 0x40,

    /* U+0053 "S" */
    0xf2, 0x72, 0x78,

    /* U+0054 "T" */
    0xf9, 0x8, 0x42, 0x10, 0x80,

    /* U+0055 "U" */
    0x99, 0x99, 0x99, 0xf0,

    /* U+0056 "V" */
    0x9a, 0xaa, 0xa6, 0x40,

    /* U+0057 "W" */
    0x99, 0x9d, 0xff, 0xf0,

    /* U+0058 "X" */
    0x9a, 0x64, 0x6a, 0x90,

    /* U+0059 "Y" */
    0x8a, 0x94, 0x42, 0x10, 0x80,

    /* U+005A "Z" */
    0xf2, 0x24, 0x48, 0xe0,

    /* U+005B "[" */
    0xea, 0xaa, 0xb0,

    /* U+005C "\\" */
    0x42, 0x10, 0x42, 0x8, 0x42, 0x8,

    /* U+005D "]" */
    0xd5, 0x55, 0x70,

    /* U+005E "^" */
    0x46, 0xaa,

    /* U+005F "_" */
    0xf8,

    /* U+0060 "`" */
    0xa0,

    /* U+0061 "a" */
    0xf0, 0xbd, 0x2f, 0x80,

    /* U+0062 "b" */
    0x88, 0xf9, 0x99, 0xf0,

    /* U+0063 "c" */
    0xf2, 0x4e,

    /* U+0064 "d" */
    0x11, 0xf9, 0x99, 0xf0,

    /* U+0065 "e" */
    0xf9, 0xf8, 0xf0,

    /* U+0066 "f" */
    0x74, 0xf4, 0x44, 0xe0,

    /* U+0067 "g" */
    0xf9, 0x9f, 0x71, 0xf0,

    /* U+0068 "h" */
    0x88, 0xf9, 0x99, 0x90,

    /* U+0069 "i" */
    0x43, 0x24, 0x98,

    /* U+006A "j" */
    0x21, 0x92, 0x49, 0xe0,

    /* U+006B "k" */
    0x88, 0xac, 0xca, 0xa0,

    /* U+006C "l" */
    0xc4, 0x44, 0x44, 0x70,

    /* U+006D "m" */
    0xfd, 0x6b, 0x5a, 0x80,

    /* U+006E "n" */
    0xf9, 0x99, 0x90,

    /* U+006F "o" */
    0xf9, 0x99, 0xf0,

    /* U+0070 "p" */
    0xf9, 0x99, 0xf8, 0x80,

    /* U+0071 "q" */
    0xf9, 0x99, 0xf1, 0x10,

    /* U+0072 "r" */
    0xf4, 0x44, 0xe0,

    /* U+0073 "s" */
    0xf3, 0x9e,

    /* U+0074 "t" */
    0x5d, 0x24, 0xc0,

    /* U+0075 "u" */
    0x99, 0x99, 0xf0,

    /* U+0076 "v" */
    0x9a, 0xa6, 0x40,

    /* U+0077 "w" */
    0x8d, 0x5a, 0xe5, 0x0,

    /* U+0078 "x" */
    0xa6, 0x4a, 0xa0,

    /* U+0079 "y" */
    0x9a, 0xa6, 0x64, 0xc0,

    /* U+007A "z" */
    0xf2, 0x48, 0xe0,

    /* U+007B "{" */
    0x69, 0x25, 0x12, 0x4c,

    /* U+007C "|" */
    0xff, 0x80,

    /* U+007D "}" */
    0xc9, 0x24, 0x52, 0x58,

    /* U+007E "~" */
    0xe0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 86, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 86, .box_w = 1, .box_h = 7, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 2, .adv_w = 86, .box_w = 3, .box_h = 2, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 3, .adv_w = 86, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 8, .adv_w = 86, .box_w = 3, .box_h = 9, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 12, .adv_w = 86, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 17, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 21, .adv_w = 86, .box_w = 1, .box_h = 2, .ofs_x = 2, .ofs_y = 5},
    {.bitmap_index = 22, .adv_w = 86, .box_w = 2, .box_h = 10, .ofs_x = 2, .ofs_y = -1},
    {.bitmap_index = 25, .adv_w = 86, .box_w = 2, .box_h = 10, .ofs_x = 2, .ofs_y = -1},
    {.bitmap_index = 28, .adv_w = 86, .box_w = 4, .box_h = 4, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 30, .adv_w = 86, .box_w = 5, .box_h = 4, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 33, .adv_w = 86, .box_w = 1, .box_h = 2, .ofs_x = 2, .ofs_y = -1},
    {.bitmap_index = 34, .adv_w = 86, .box_w = 3, .box_h = 1, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 35, .adv_w = 86, .box_w = 1, .box_h = 1, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 36, .adv_w = 86, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 42, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 46, .adv_w = 86, .box_w = 3, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 49, .adv_w = 86, .box_w = 3, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 52, .adv_w = 86, .box_w = 3, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 55, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 59, .adv_w = 86, .box_w = 3, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 62, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 66, .adv_w = 86, .box_w = 3, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 69, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 73, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 77, .adv_w = 86, .box_w = 1, .box_h = 5, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 78, .adv_w = 86, .box_w = 1, .box_h = 6, .ofs_x = 2, .ofs_y = -1},
    {.bitmap_index = 79, .adv_w = 86, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 82, .adv_w = 86, .box_w = 4, .box_h = 3, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 84, .adv_w = 86, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 87, .adv_w = 86, .box_w = 3, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 90, .adv_w = 86, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 95, .adv_w = 86, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 100, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 104, .adv_w = 86, .box_w = 3, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 107, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 111, .adv_w = 86, .box_w = 3, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 114, .adv_w = 86, .box_w = 3, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 117, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 121, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 125, .adv_w = 86, .box_w = 3, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 128, .adv_w = 86, .box_w = 3, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 131, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 135, .adv_w = 86, .box_w = 3, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 138, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 142, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 146, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 150, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 154, .adv_w = 86, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 159, .adv_w = 86, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 164, .adv_w = 86, .box_w = 3, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 167, .adv_w = 86, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 172, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 176, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 180, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 184, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 188, .adv_w = 86, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 193, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 197, .adv_w = 86, .box_w = 2, .box_h = 10, .ofs_x = 2, .ofs_y = -1},
    {.bitmap_index = 200, .adv_w = 86, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 206, .adv_w = 86, .box_w = 2, .box_h = 10, .ofs_x = 2, .ofs_y = -1},
    {.bitmap_index = 209, .adv_w = 86, .box_w = 4, .box_h = 4, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 211, .adv_w = 86, .box_w = 5, .box_h = 1, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 212, .adv_w = 86, .box_w = 2, .box_h = 2, .ofs_x = 2, .ofs_y = 6},
    {.bitmap_index = 213, .adv_w = 86, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 217, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 221, .adv_w = 86, .box_w = 3, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 223, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 227, .adv_w = 86, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 230, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 234, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 238, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 242, .adv_w = 86, .box_w = 3, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 245, .adv_w = 86, .box_w = 3, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 249, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 253, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 257, .adv_w = 86, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 261, .adv_w = 86, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 264, .adv_w = 86, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 267, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 271, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 275, .adv_w = 86, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 278, .adv_w = 86, .box_w = 3, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 280, .adv_w = 86, .box_w = 3, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 283, .adv_w = 86, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 286, .adv_w = 86, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 289, .adv_w = 86, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 293, .adv_w = 86, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 296, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 300, .adv_w = 86, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 303, .adv_w = 86, .box_w = 3, .box_h = 10, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 307, .adv_w = 86, .box_w = 1, .box_h = 9, .ofs_x = 2, .ofs_y = -1},
    {.bitmap_index = 309, .adv_w = 86, .box_w = 3, .box_h = 10, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 313, .adv_w = 86, .box_w = 3, .box_h = 1, .ofs_x = 1, .ofs_y = 3}
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
const lv_font_t pda_font_mono_10 = {
#else
lv_font_t pda_font_mono_10 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 11,          /*The maximum line height required by the font*/
    .base_line = 2,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if PDA_FONT_MONO_10*/

