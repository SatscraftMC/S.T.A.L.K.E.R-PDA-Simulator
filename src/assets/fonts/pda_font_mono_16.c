/*******************************************************************************
 * Size: 16 px
 * Bpp: 1
 * Opts: --font /home/aj/pda-project/app/build/fonts-src/ShareTechMono-Regular.ttf --size 16 --bpp 1 --format lvgl --lv-include lvgl.h --no-compress -r 0x20-0x7F --lv-font-name pda_font_mono_16 -o /home/aj/pda-project/app/src/assets/fonts/pda_font_mono_16.c
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl.h"
#endif

#ifndef PDA_FONT_MONO_16
#define PDA_FONT_MONO_16 1
#endif

#if PDA_FONT_MONO_16

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0021 "!" */
    0xff, 0x20,

    /* U+0022 "\"" */
    0x99, 0x99,

    /* U+0023 "#" */
    0x49, 0x24, 0xbf, 0x49, 0x24, 0xbf, 0x49, 0x24,
    0x80,

    /* U+0024 "$" */
    0x21, 0x1f, 0x8, 0x42, 0xe, 0x8, 0x42, 0x1f,
    0x10, 0x80,

    /* U+0025 "%" */
    0x30, 0x24, 0x12, 0x9, 0x3, 0x3, 0xfc, 0x18,
    0x12, 0x9, 0x4, 0x81, 0x80,

    /* U+0026 "&" */
    0x72, 0x8, 0x20, 0x7e, 0x28, 0xa2, 0x8a, 0x27,
    0x80,

    /* U+0027 "'" */
    0xf0,

    /* U+0028 "(" */
    0x4a, 0x49, 0x24, 0x92, 0x48, 0x90,

    /* U+0029 ")" */
    0x48, 0x92, 0x49, 0x24, 0x92, 0x90,

    /* U+002A "*" */
    0x10, 0x23, 0xf1, 0x86, 0x89, 0x80,

    /* U+002B "+" */
    0x10, 0x4f, 0xc4, 0x10, 0x40,

    /* U+002C "," */
    0x57,

    /* U+002D "-" */
    0xf8,

    /* U+002E "." */
    0x80,

    /* U+002F "/" */
    0x6, 0x8, 0x10, 0x60, 0x81, 0x4, 0x8, 0x30,
    0x40, 0x83, 0x4, 0x18, 0x20, 0x0,

    /* U+0030 "0" */
    0x7a, 0x18, 0x63, 0x9e, 0xde, 0x71, 0x86, 0x17,
    0x80,

    /* U+0031 "1" */
    0x65, 0x8, 0x42, 0x10, 0x84, 0x21, 0x3e,

    /* U+0032 "2" */
    0xf0, 0x42, 0x10, 0x8c, 0xc4, 0x66, 0x3e,

    /* U+0033 "3" */
    0xf0, 0x42, 0x10, 0xb8, 0x21, 0x8, 0x7c,

    /* U+0034 "4" */
    0x30, 0x40, 0x83, 0x6, 0x89, 0x12, 0x7f, 0x8,
    0x10, 0x20,

    /* U+0035 "5" */
    0xfc, 0x21, 0xf, 0x4, 0x21, 0x8, 0x7c,

    /* U+0036 "6" */
    0x74, 0x21, 0xf, 0x46, 0x31, 0x8c, 0x5c,

    /* U+0037 "7" */
    0xf8, 0x46, 0x21, 0x18, 0xc4, 0x23, 0x10,

    /* U+0038 "8" */
    0x7a, 0x18, 0x61, 0x85, 0xe8, 0x61, 0x86, 0x17,
    0x80,

    /* U+0039 "9" */
    0x74, 0x63, 0x18, 0xc5, 0xe1, 0x8, 0x5c,

    /* U+003A ":" */
    0x81,

    /* U+003B ";" */
    0x40, 0x5, 0x70,

    /* U+003C "<" */
    0x4, 0x77, 0x20, 0xe0, 0xe0, 0xc0,

    /* U+003D "=" */
    0xfc, 0x0, 0x3f,

    /* U+003E ">" */
    0x83, 0x83, 0x81, 0x1d, 0xcc, 0x0,

    /* U+003F "?" */
    0xe1, 0x11, 0x36, 0x44, 0x0, 0x40,

    /* U+0040 "@" */
    0x7e, 0x81, 0x9d, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
    0xbf, 0x80, 0x7c,

    /* U+0041 "A" */
    0x30, 0x70, 0xa1, 0x46, 0x89, 0x91, 0x3e, 0xc5,
    0xe, 0x18,

    /* U+0042 "B" */
    0xfa, 0x18, 0x61, 0x87, 0xe8, 0x61, 0x86, 0x1f,
    0x80,

    /* U+0043 "C" */
    0x7c, 0x21, 0x8, 0x42, 0x10, 0x84, 0x1e,

    /* U+0044 "D" */
    0xfa, 0x18, 0x61, 0x86, 0x18, 0x61, 0x86, 0x1f,
    0x80,

    /* U+0045 "E" */
    0xfc, 0x21, 0x8, 0x7e, 0x10, 0x84, 0x3e,

    /* U+0046 "F" */
    0xfc, 0x21, 0x8, 0x7e, 0x10, 0x84, 0x20,

    /* U+0047 "G" */
    0x7c, 0x21, 0x8, 0x4e, 0x31, 0x8c, 0x5e,

    /* U+0048 "H" */
    0x86, 0x18, 0x61, 0x87, 0xf8, 0x61, 0x86, 0x18,
    0x40,

    /* U+0049 "I" */
    0xf2, 0x22, 0x22, 0x22, 0x22, 0xf0,

    /* U+004A "J" */
    0x71, 0x11, 0x11, 0x11, 0x11, 0xe0,

    /* U+004B "K" */
    0x8a, 0x6b, 0x28, 0xe3, 0x8a, 0x2c, 0x92, 0x68,
    0x80,

    /* U+004C "L" */
    0x84, 0x21, 0x8, 0x42, 0x10, 0x84, 0x3e,

    /* U+004D "M" */
    0xc7, 0xcf, 0xbd, 0x5a, 0xb7, 0x64, 0xc1, 0x83,
    0x6, 0x8,

    /* U+004E "N" */
    0xc7, 0x9e, 0x69, 0xa6, 0xd9, 0x65, 0x9e, 0x78,
    0xc0,

    /* U+004F "O" */
    0x7a, 0x18, 0x61, 0x86, 0x18, 0x61, 0x86, 0x17,
    0x80,

    /* U+0050 "P" */
    0xfa, 0x18, 0x61, 0x86, 0x1f, 0xa0, 0x82, 0x8,
    0x0,

    /* U+0051 "Q" */
    0x79, 0xa, 0x14, 0x28, 0x50, 0xa1, 0x42, 0x85,
    0x9, 0xf0, 0x0,

    /* U+0052 "R" */
    0xf9, 0xa, 0x14, 0x28, 0x5f, 0x26, 0x46, 0x8d,
    0xa, 0x18,

    /* U+0053 "S" */
    0x7c, 0x21, 0x8, 0x38, 0x21, 0x8, 0x7c,

    /* U+0054 "T" */
    0xfc, 0x41, 0x4, 0x10, 0x41, 0x4, 0x10, 0x41,
    0x0,

    /* U+0055 "U" */
    0x86, 0x18, 0x61, 0x86, 0x18, 0x61, 0x86, 0x17,
    0x80,

    /* U+0056 "V" */
    0x87, 0x8b, 0x12, 0x24, 0xc9, 0x1a, 0x34, 0x38,
    0x70, 0xc0,

    /* U+0057 "W" */
    0x86, 0x18, 0x61, 0x86, 0xdb, 0x6d, 0xff, 0x3c,
    0xc0,

    /* U+0058 "X" */
    0xc4, 0x99, 0xa1, 0xc3, 0x6, 0xe, 0x14, 0x6c,
    0x8b, 0x18,

    /* U+0059 "Y" */
    0xc6, 0x89, 0xb1, 0x43, 0x82, 0x4, 0x8, 0x10,
    0x20, 0x40,

    /* U+005A "Z" */
    0x7c, 0x30, 0x86, 0x18, 0xc3, 0x18, 0x63, 0xf,
    0xc0,

    /* U+005B "[" */
    0xf2, 0x49, 0x24, 0x92, 0x49, 0x38,

    /* U+005C "\\" */
    0x81, 0x81, 0x2, 0x6, 0x4, 0xc, 0x8, 0x10,
    0x30, 0x20, 0x40, 0x40, 0x81, 0x80,

    /* U+005D "]" */
    0xe4, 0x92, 0x49, 0x24, 0x92, 0x78,

    /* U+005E "^" */
    0x30, 0xe2, 0x9a, 0x4f, 0x10,

    /* U+005F "_" */
    0xff,

    /* U+0060 "`" */
    0xcc,

    /* U+0061 "a" */
    0xf0, 0x20, 0x9e, 0x8a, 0x28, 0xbf,

    /* U+0062 "b" */
    0x82, 0x8, 0x3e, 0x86, 0x18, 0x61, 0x86, 0x1f,
    0x80,

    /* U+0063 "c" */
    0x7c, 0x21, 0x8, 0x42, 0xf,

    /* U+0064 "d" */
    0x4, 0x10, 0x5f, 0x86, 0x18, 0x61, 0x86, 0x17,
    0xc0,

    /* U+0065 "e" */
    0x74, 0x63, 0xf8, 0x42, 0xf,

    /* U+0066 "f" */
    0x39, 0x9, 0xf2, 0x10, 0x84, 0x21, 0x3e,

    /* U+0067 "g" */
    0x7e, 0x18, 0x61, 0x85, 0xe4, 0x1e, 0x4, 0x1f,
    0x80,

    /* U+0068 "h" */
    0x82, 0x8, 0x3e, 0x86, 0x18, 0x61, 0x86, 0x18,
    0x40,

    /* U+0069 "i" */
    0x20, 0x1, 0xc2, 0x10, 0x84, 0x21, 0xe,

    /* U+006A "j" */
    0x10, 0x7, 0x11, 0x11, 0x11, 0x11, 0x1f,

    /* U+006B "k" */
    0x84, 0x21, 0x3b, 0x73, 0x94, 0xb4, 0xe6,

    /* U+006C "l" */
    0xe0, 0x82, 0x8, 0x20, 0x82, 0x8, 0x20, 0x83,
    0xc0,

    /* U+006D "m" */
    0xff, 0x26, 0x4c, 0x99, 0x32, 0x64, 0xc9,

    /* U+006E "n" */
    0xfa, 0x18, 0x61, 0x86, 0x18, 0x61,

    /* U+006F "o" */
    0x7a, 0x18, 0x61, 0x86, 0x18, 0x5e,

    /* U+0070 "p" */
    0xfa, 0x18, 0x61, 0x86, 0x18, 0x7e, 0x82, 0x8,
    0x0,

    /* U+0071 "q" */
    0x7e, 0x18, 0x61, 0x86, 0x18, 0x5f, 0x4, 0x10,
    0x40,

    /* U+0072 "r" */
    0xfa, 0x10, 0x84, 0x21, 0x1e,

    /* U+0073 "s" */
    0x7c, 0x20, 0xe0, 0x84, 0x3e,

    /* U+0074 "t" */
    0x21, 0x3e, 0x42, 0x10, 0x84, 0x21, 0xc0,

    /* U+0075 "u" */
    0x8c, 0x63, 0x18, 0xc6, 0x2f,

    /* U+0076 "v" */
    0xc5, 0x89, 0x12, 0x66, 0x85, 0xe, 0x18,

    /* U+0077 "w" */
    0x83, 0x26, 0x5d, 0xaf, 0xcd, 0x9b, 0x36,

    /* U+0078 "x" */
    0x44, 0xd0, 0xe1, 0x83, 0x5, 0x13, 0x62,

    /* U+0079 "y" */
    0x87, 0x14, 0xd2, 0x68, 0xa3, 0x8c, 0x10, 0xc6,
    0x0,

    /* U+007A "z" */
    0xf8, 0xc4, 0x66, 0x23, 0x1f,

    /* U+007B "{" */
    0x74, 0x44, 0x44, 0xcc, 0x44, 0x44, 0x44, 0x70,

    /* U+007C "|" */
    0xff, 0xfe,

    /* U+007D "}" */
    0xe1, 0x8, 0x42, 0x10, 0xe7, 0x21, 0x8, 0x42,
    0x13, 0x80,

    /* U+007E "~" */
    0x6c, 0xc0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 138, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 138, .box_w = 1, .box_h = 11, .ofs_x = 4, .ofs_y = 0},
    {.bitmap_index = 3, .adv_w = 138, .box_w = 4, .box_h = 4, .ofs_x = 2, .ofs_y = 7},
    {.bitmap_index = 5, .adv_w = 138, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 14, .adv_w = 138, .box_w = 5, .box_h = 15, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 24, .adv_w = 138, .box_w = 9, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 37, .adv_w = 138, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 46, .adv_w = 138, .box_w = 1, .box_h = 4, .ofs_x = 4, .ofs_y = 7},
    {.bitmap_index = 47, .adv_w = 138, .box_w = 3, .box_h = 15, .ofs_x = 3, .ofs_y = -2},
    {.bitmap_index = 53, .adv_w = 138, .box_w = 3, .box_h = 15, .ofs_x = 3, .ofs_y = -2},
    {.bitmap_index = 59, .adv_w = 138, .box_w = 7, .box_h = 6, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 65, .adv_w = 138, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 70, .adv_w = 138, .box_w = 2, .box_h = 4, .ofs_x = 3, .ofs_y = -2},
    {.bitmap_index = 71, .adv_w = 138, .box_w = 5, .box_h = 1, .ofs_x = 2, .ofs_y = 5},
    {.bitmap_index = 72, .adv_w = 138, .box_w = 1, .box_h = 1, .ofs_x = 4, .ofs_y = 0},
    {.bitmap_index = 73, .adv_w = 138, .box_w = 7, .box_h = 15, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 87, .adv_w = 138, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 96, .adv_w = 138, .box_w = 5, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 103, .adv_w = 138, .box_w = 5, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 110, .adv_w = 138, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 117, .adv_w = 138, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 127, .adv_w = 138, .box_w = 5, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 134, .adv_w = 138, .box_w = 5, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 141, .adv_w = 138, .box_w = 5, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 148, .adv_w = 138, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 157, .adv_w = 138, .box_w = 5, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 164, .adv_w = 138, .box_w = 1, .box_h = 8, .ofs_x = 4, .ofs_y = 0},
    {.bitmap_index = 165, .adv_w = 138, .box_w = 2, .box_h = 10, .ofs_x = 3, .ofs_y = -2},
    {.bitmap_index = 168, .adv_w = 138, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 174, .adv_w = 138, .box_w = 6, .box_h = 4, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 177, .adv_w = 138, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 183, .adv_w = 138, .box_w = 4, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 189, .adv_w = 138, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 200, .adv_w = 138, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 210, .adv_w = 138, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 219, .adv_w = 138, .box_w = 5, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 226, .adv_w = 138, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 235, .adv_w = 138, .box_w = 5, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 242, .adv_w = 138, .box_w = 5, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 249, .adv_w = 138, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 256, .adv_w = 138, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 265, .adv_w = 138, .box_w = 4, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 271, .adv_w = 138, .box_w = 4, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 277, .adv_w = 138, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 286, .adv_w = 138, .box_w = 5, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 293, .adv_w = 138, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 303, .adv_w = 138, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 312, .adv_w = 138, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 321, .adv_w = 138, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 330, .adv_w = 138, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 341, .adv_w = 138, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 351, .adv_w = 138, .box_w = 5, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 358, .adv_w = 138, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 367, .adv_w = 138, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 376, .adv_w = 138, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 386, .adv_w = 138, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 395, .adv_w = 138, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 405, .adv_w = 138, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 415, .adv_w = 138, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 424, .adv_w = 138, .box_w = 3, .box_h = 15, .ofs_x = 3, .ofs_y = -2},
    {.bitmap_index = 430, .adv_w = 138, .box_w = 7, .box_h = 15, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 444, .adv_w = 138, .box_w = 3, .box_h = 15, .ofs_x = 3, .ofs_y = -2},
    {.bitmap_index = 450, .adv_w = 138, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 7},
    {.bitmap_index = 455, .adv_w = 138, .box_w = 8, .box_h = 1, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 456, .adv_w = 138, .box_w = 3, .box_h = 2, .ofs_x = 3, .ofs_y = 9},
    {.bitmap_index = 457, .adv_w = 138, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 463, .adv_w = 138, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 472, .adv_w = 138, .box_w = 5, .box_h = 8, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 477, .adv_w = 138, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 486, .adv_w = 138, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 491, .adv_w = 138, .box_w = 5, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 498, .adv_w = 138, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 507, .adv_w = 138, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 516, .adv_w = 138, .box_w = 5, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 523, .adv_w = 138, .box_w = 4, .box_h = 14, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 530, .adv_w = 138, .box_w = 5, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 537, .adv_w = 138, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 546, .adv_w = 138, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 553, .adv_w = 138, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 559, .adv_w = 138, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 565, .adv_w = 138, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 574, .adv_w = 138, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 583, .adv_w = 138, .box_w = 5, .box_h = 8, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 588, .adv_w = 138, .box_w = 5, .box_h = 8, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 593, .adv_w = 138, .box_w = 5, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 600, .adv_w = 138, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 605, .adv_w = 138, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 612, .adv_w = 138, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 619, .adv_w = 138, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 626, .adv_w = 138, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 635, .adv_w = 138, .box_w = 5, .box_h = 8, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 640, .adv_w = 138, .box_w = 4, .box_h = 15, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 648, .adv_w = 138, .box_w = 1, .box_h = 15, .ofs_x = 4, .ofs_y = -2},
    {.bitmap_index = 650, .adv_w = 138, .box_w = 5, .box_h = 15, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 660, .adv_w = 138, .box_w = 5, .box_h = 2, .ofs_x = 2, .ofs_y = 4}
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
const lv_font_t pda_font_mono_16 = {
#else
lv_font_t pda_font_mono_16 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 16,          /*The maximum line height required by the font*/
    .base_line = 3,             /*Baseline measured from the bottom of the line*/
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



#endif /*#if PDA_FONT_MONO_16*/

