/*******************************************************************************
 * Size: 10 px
 * Bpp: 1
 * Opts: --font /home/aj/pda-project/app/build/fonts-src/VT323-Regular.ttf --size 10 --bpp 1 --format lvgl --lv-include lvgl.h --no-compress -r 0x100-0x17F --lv-font-name pda_font_fallback_10 -o /home/aj/pda-project/app/src/assets/fonts/pda_font_fallback_10.c
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl.h"
#endif

#ifndef PDA_FONT_FALLBACK_10
#define PDA_FONT_FALLBACK_10 1
#endif

#if PDA_FONT_FALLBACK_10

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0100 "Ā" */
    0x66, 0x66, 0x6a, 0x90,

    /* U+0101 "ā" */
    0x60, 0x66, 0xa5,

    /* U+0102 "Ă" */
    0x96, 0x66, 0x66, 0xa9,

    /* U+0103 "ă" */
    0x96, 0x6, 0x6a, 0x50,

    /* U+0104 "Ą" */
    0x66, 0x66, 0x69, 0x21,

    /* U+0105 "ą" */
    0x62, 0x6f, 0x21,

    /* U+0106 "Ć" */
    0x22, 0x65, 0x88, 0x56,

    /* U+0107 "ć" */
    0x22, 0x6, 0xd5, 0x60,

    /* U+0108 "Ĉ" */
    0x26, 0x65, 0x88, 0x56,

    /* U+0109 "ĉ" */
    0x26, 0x6, 0xd5, 0x60,

    /* U+010A "Ċ" */
    0x26, 0x58, 0x85, 0x60,

    /* U+010B "ċ" */
    0x20, 0x6d, 0x56,

    /* U+010C "Č" */
    0x62, 0x65, 0x88, 0x56,

    /* U+010D "č" */
    0x62, 0x6, 0xd5, 0x60,

    /* U+010E "Ď" */
    0x66, 0xea, 0x99, 0xae,

    /* U+010F "ď" */
    0x32, 0x6a, 0xa6,

    /* U+0110 "Đ" */
    0xe6, 0x5d, 0x6e,

    /* U+0111 "đ" */
    0x27, 0x2e, 0xa6,

    /* U+0112 "Ē" */
    0xfa, 0xec,

    /* U+0113 "ē" */
    0x60, 0x6f, 0x46,

    /* U+0114 "Ĕ" */
    0x96, 0x64, 0x46, 0x46,

    /* U+0115 "ĕ" */
    0x96, 0x6, 0xf4, 0x60,

    /* U+0116 "Ė" */
    0xba, 0xec,

    /* U+0117 "ė" */
    0x20, 0x6f, 0x46,

    /* U+0118 "Ę" */
    0xd2, 0x69, 0x91,

    /* U+0119 "ę" */
    0x65, 0xf6, 0x21,

    /* U+011A "Ě" */
    0xfe, 0xbb,

    /* U+011B "ě" */
    0x62, 0x6, 0xf4, 0x60,

    /* U+011C "Ĝ" */
    0x26, 0x65, 0x8b, 0x56,

    /* U+011D "ĝ" */
    0x60, 0x75, 0x97, 0x26,

    /* U+011E "Ğ" */
    0x96, 0x65, 0x8b, 0x56,

    /* U+011F "ğ" */
    0xf0, 0x75, 0x97, 0x26,

    /* U+0120 "Ġ" */
    0x26, 0x58, 0xb5, 0x60,

    /* U+0121 "ġ" */
    0x20, 0x75, 0x97, 0x26,

    /* U+0122 "Ģ" */
    0x66, 0xdb, 0x56, 0x2, 0x20,

    /* U+0123 "ģ" */
    0x2, 0x7, 0x59, 0x72, 0x60,

    /* U+0124 "Ĥ" */
    0x66, 0x99, 0x9f, 0x99,

    /* U+0125 "ĥ" */
    0x66, 0x88, 0xed, 0x99,

    /* U+0126 "Ħ" */
    0x6f, 0x66, 0x66,

    /* U+0127 "ħ" */
    0x4e, 0x46, 0x55,

    /* U+0128 "Ĩ" */
    0x5a, 0x64, 0x44, 0x46,

    /* U+0129 "ĩ" */
    0x5a, 0x4, 0x44, 0x60,

    /* U+012A "Ī" */
    0xfa, 0xac,

    /* U+012B "ī" */
    0xca, 0xb0,

    /* U+012C "Ĭ" */
    0x96, 0x64, 0x44, 0x46,

    /* U+012D "ĭ" */
    0x96, 0x4, 0x44, 0x60,

    /* U+012E "Į" */
    0xea, 0xb9,

    /* U+012F "į" */
    0x8a, 0xb9,

    /* U+0130 "İ" */
    0xba, 0xac,

    /* U+0131 "ı" */
    0xab,

    /* U+0132 "Ĳ" */
    0x99, 0x99, 0x9a,

    /* U+0133 "ĳ" */
    0x63, 0xb6, 0xca,

    /* U+0134 "Ĵ" */
    0x25, 0x62, 0x22, 0xa4,

    /* U+0135 "ĵ" */
    0xcd, 0x5a,

    /* U+0136 "Ķ" */
    0x9a, 0xac, 0xab, 0x4, 0x0,

    /* U+0137 "ķ" */
    0x88, 0xaa, 0xeb, 0x4, 0x0,

    /* U+0138 "ĸ" */
    0xbe, 0xaa,

    /* U+0139 "Ĺ" */
    0x52, 0x49, 0x27,

    /* U+013A "ĺ" */
    0x6a, 0xab,

    /* U+013B "Ļ" */
    0x92, 0x49, 0xc2, 0x40,

    /* U+013C "ļ" */
    0xaa, 0xb2, 0x0,

    /* U+013D "Ľ" */
    0xd2, 0x49, 0xc0,

    /* U+013E "ľ" */
    0xea, 0xb0,

    /* U+013F "Ŀ" */
    0x92, 0x69, 0xc0,

    /* U+0140 "ŀ" */
    0x92, 0x59, 0x80,

    /* U+0141 "Ł" */
    0x44, 0x6c, 0x47,

    /* U+0142 "ł" */
    0xa6, 0xb0,

    /* U+0143 "Ń" */
    0x24, 0x9d, 0xdb, 0xb9,

    /* U+0144 "ń" */
    0x24, 0xe, 0xd9, 0x90,

    /* U+0145 "Ņ" */
    0x99, 0xdd, 0xbb, 0x4, 0x0,

    /* U+0146 "ņ" */
    0xed, 0x99, 0x4, 0x0,

    /* U+0147 "Ň" */
    0x66, 0x9d, 0xdb, 0xb9,

    /* U+0148 "ň" */
    0x66, 0xe, 0xd9, 0x90,

    /* U+0149 "ŉ" */
    0x88, 0x65, 0x55,

    /* U+014A "Ŋ" */
    0x99, 0xdd, 0xbb, 0x12,

    /* U+014B "ŋ" */
    0xed, 0x99, 0x12,

    /* U+014C "Ō" */
    0x66, 0x69, 0x96, 0x60,

    /* U+014D "ō" */
    0x60, 0x6f, 0x66,

    /* U+014E "Ŏ" */
    0x96, 0x66, 0x99, 0x66,

    /* U+014F "ŏ" */
    0x96, 0x6, 0xf6, 0x60,

    /* U+0150 "Ő" */
    0x66, 0x66, 0x99, 0x66,

    /* U+0151 "ő" */
    0x66, 0x6, 0xf6, 0x60,

    /* U+0152 "Œ" */
    0x76, 0xab, 0x67,

    /* U+0153 "œ" */
    0x6b, 0xa7,

    /* U+0154 "Ŕ" */
    0x24, 0xe9, 0xae, 0xa9,

    /* U+0155 "ŕ" */
    0x24, 0xa, 0x54, 0xc0,

    /* U+0156 "Ŗ" */
    0xea, 0x9e, 0xa9, 0x4, 0x0,

    /* U+0157 "ŗ" */
    0xa5, 0x4c, 0x4, 0x40,

    /* U+0158 "Ř" */
    0x66, 0xe9, 0xae, 0xa9,

    /* U+0159 "ř" */
    0x66, 0xa, 0x54, 0xc0,

    /* U+015A "Ś" */
    0x24, 0x69, 0x87, 0x96,

    /* U+015B "ś" */
    0x63, 0xdc,

    /* U+015C "Ŝ" */
    0x66, 0x69, 0x87, 0x96,

    /* U+015D "ŝ" */
    0xf3, 0xdc,

    /* U+015E "Ş" */
    0x6a, 0x96, 0x1f, 0x24,

    /* U+015F "ş" */
    0xef, 0x60,

    /* U+0160 "Š" */
    0x66, 0x69, 0x87, 0x96,

    /* U+0161 "š" */
    0xf3, 0xdc,

    /* U+0162 "Ţ" */
    0xf4, 0x44, 0x44, 0x24,

    /* U+0163 "ţ" */
    0x4b, 0xa4, 0x49,

    /* U+0164 "Ť" */
    0x66, 0xf4, 0x44, 0x44,

    /* U+0165 "ť" */
    0x2d, 0x74, 0x88,

    /* U+0166 "Ŧ" */
    0xf4, 0x46, 0x44,

    /* U+0167 "ŧ" */
    0x4b, 0xb4, 0x40,

    /* U+0168 "Ũ" */
    0x5a, 0x99, 0x99, 0x96,

    /* U+0169 "ũ" */
    0x5a, 0x9, 0x9f, 0x70,

    /* U+016A "Ū" */
    0x69, 0x99, 0x99, 0x60,

    /* U+016B "ū" */
    0x60, 0x99, 0xf7,

    /* U+016C "Ŭ" */
    0x96, 0x99, 0x99, 0x96,

    /* U+016D "ŭ" */
    0x96, 0x9, 0x9f, 0x70,

    /* U+016E "Ů" */
    0x66, 0x69, 0x99, 0x99, 0x60,

    /* U+016F "ů" */
    0x66, 0x9, 0x9f, 0x70,

    /* U+0170 "Ű" */
    0x66, 0x99, 0x99, 0x96,

    /* U+0171 "ű" */
    0x66, 0x9, 0x9f, 0x70,

    /* U+0172 "Ų" */
    0x99, 0x99, 0x96, 0x42,

    /* U+0173 "ų" */
    0x99, 0xf7, 0x21,

    /* U+0174 "Ŵ" */
    0x66, 0xdd, 0xd6, 0x66,

    /* U+0175 "ŵ" */
    0x66, 0xd, 0x66, 0x60,

    /* U+0176 "Ŷ" */
    0x66, 0x9a, 0x64, 0x44,

    /* U+0177 "ŷ" */
    0x60, 0x9a, 0x66, 0x44,

    /* U+0178 "Ÿ" */
    0x69, 0xa6, 0x44, 0x40,

    /* U+0179 "Ź" */
    0x6d, 0x7b,

    /* U+017A "ź" */
    0x63, 0xcc,

    /* U+017B "Ż" */
    0xb5, 0xec,

    /* U+017C "ż" */
    0x8f, 0x30,

    /* U+017D "Ž" */
    0xfd, 0x7b,

    /* U+017E "ž" */
    0xf3, 0xcc
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 64, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 4, .adv_w = 64, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 7, .adv_w = 64, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 11, .adv_w = 64, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 15, .adv_w = 64, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 19, .adv_w = 64, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 22, .adv_w = 64, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 26, .adv_w = 64, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 30, .adv_w = 64, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 34, .adv_w = 64, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 38, .adv_w = 64, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 42, .adv_w = 64, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 45, .adv_w = 64, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 49, .adv_w = 64, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 53, .adv_w = 64, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 57, .adv_w = 64, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 60, .adv_w = 64, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 63, .adv_w = 64, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 66, .adv_w = 64, .box_w = 2, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 68, .adv_w = 64, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 71, .adv_w = 64, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 75, .adv_w = 64, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 79, .adv_w = 64, .box_w = 2, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 81, .adv_w = 64, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 84, .adv_w = 64, .box_w = 3, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 87, .adv_w = 64, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 90, .adv_w = 64, .box_w = 2, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 92, .adv_w = 64, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 96, .adv_w = 64, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 100, .adv_w = 64, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 104, .adv_w = 64, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 108, .adv_w = 64, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 112, .adv_w = 64, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 116, .adv_w = 64, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 120, .adv_w = 64, .box_w = 4, .box_h = 9, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 125, .adv_w = 64, .box_w = 4, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 130, .adv_w = 64, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 134, .adv_w = 64, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 138, .adv_w = 64, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 141, .adv_w = 64, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 144, .adv_w = 64, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 148, .adv_w = 64, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 152, .adv_w = 64, .box_w = 2, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 154, .adv_w = 64, .box_w = 2, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 156, .adv_w = 64, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 160, .adv_w = 64, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 164, .adv_w = 64, .box_w = 2, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 166, .adv_w = 64, .box_w = 2, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 168, .adv_w = 64, .box_w = 2, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 170, .adv_w = 64, .box_w = 2, .box_h = 4, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 171, .adv_w = 64, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 174, .adv_w = 64, .box_w = 3, .box_h = 8, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 177, .adv_w = 64, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 181, .adv_w = 64, .box_w = 2, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 183, .adv_w = 64, .box_w = 4, .box_h = 9, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 188, .adv_w = 64, .box_w = 4, .box_h = 9, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 193, .adv_w = 64, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 195, .adv_w = 64, .box_w = 3, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 198, .adv_w = 64, .box_w = 2, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 200, .adv_w = 64, .box_w = 3, .box_h = 9, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 204, .adv_w = 64, .box_w = 2, .box_h = 9, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 207, .adv_w = 64, .box_w = 3, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 210, .adv_w = 64, .box_w = 2, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 212, .adv_w = 64, .box_w = 3, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 215, .adv_w = 64, .box_w = 3, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 218, .adv_w = 64, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 221, .adv_w = 64, .box_w = 2, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 223, .adv_w = 64, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 227, .adv_w = 64, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 231, .adv_w = 64, .box_w = 4, .box_h = 9, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 236, .adv_w = 64, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 240, .adv_w = 64, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 244, .adv_w = 64, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 248, .adv_w = 64, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 251, .adv_w = 64, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 255, .adv_w = 64, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 258, .adv_w = 64, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 262, .adv_w = 64, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 265, .adv_w = 64, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 269, .adv_w = 64, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 273, .adv_w = 64, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 277, .adv_w = 64, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 281, .adv_w = 64, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 284, .adv_w = 64, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 286, .adv_w = 64, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 290, .adv_w = 64, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 294, .adv_w = 64, .box_w = 4, .box_h = 9, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 299, .adv_w = 64, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 303, .adv_w = 64, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 307, .adv_w = 64, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 311, .adv_w = 64, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 315, .adv_w = 64, .box_w = 2, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 317, .adv_w = 64, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 321, .adv_w = 64, .box_w = 2, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 323, .adv_w = 64, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 327, .adv_w = 64, .box_w = 2, .box_h = 6, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 329, .adv_w = 64, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 333, .adv_w = 64, .box_w = 2, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 335, .adv_w = 64, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 339, .adv_w = 64, .box_w = 3, .box_h = 8, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 342, .adv_w = 64, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 346, .adv_w = 64, .box_w = 3, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 349, .adv_w = 64, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 352, .adv_w = 64, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 355, .adv_w = 64, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 359, .adv_w = 64, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 363, .adv_w = 64, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 367, .adv_w = 64, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 370, .adv_w = 64, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 374, .adv_w = 64, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 378, .adv_w = 64, .box_w = 4, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 383, .adv_w = 64, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 387, .adv_w = 64, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 391, .adv_w = 64, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 395, .adv_w = 64, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 399, .adv_w = 64, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 402, .adv_w = 64, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 406, .adv_w = 64, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 410, .adv_w = 64, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 414, .adv_w = 64, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 418, .adv_w = 64, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 422, .adv_w = 64, .box_w = 2, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 424, .adv_w = 64, .box_w = 2, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 426, .adv_w = 64, .box_w = 2, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 428, .adv_w = 64, .box_w = 2, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 430, .adv_w = 64, .box_w = 2, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 432, .adv_w = 64, .box_w = 2, .box_h = 7, .ofs_x = 1, .ofs_y = 0}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/



/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 256, .range_length = 127, .glyph_id_start = 1,
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
const lv_font_t pda_font_fallback_10 = {
#else
lv_font_t pda_font_fallback_10 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 12,          /*The maximum line height required by the font*/
    .base_line = 3,             /*Baseline measured from the bottom of the line*/
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



#endif /*#if PDA_FONT_FALLBACK_10*/

