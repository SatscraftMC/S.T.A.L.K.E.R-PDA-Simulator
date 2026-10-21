/*******************************************************************************
 * Size: 16 px
 * Bpp: 1
 * Opts: --font /home/aj/pda-project/app/build/fonts-src/VT323-Regular.ttf --size 16 --bpp 1 --format lvgl --lv-include lvgl.h --no-compress -r 0x100-0x17F --lv-font-name pda_font_fallback_16 -o /home/aj/pda-project/app/src/assets/fonts/pda_font_fallback_16.c
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl.h"
#endif

#ifndef PDA_FONT_FALLBACK_16
#define PDA_FONT_FALLBACK_16 1
#endif

#if PDA_FONT_FALLBACK_16

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0100 "Ā" */
    0x70, 0xc, 0x67, 0x29, 0x49, 0x7c, 0x63, 0x10,

    /* U+0101 "ā" */
    0xf0, 0x0, 0xe1, 0xb, 0xd2, 0x97, 0x40,

    /* U+0102 "Ă" */
    0x88, 0x1c, 0x3, 0x19, 0xca, 0x52, 0x5f, 0x18,
    0xc4,

    /* U+0103 "ă" */
    0x8b, 0x80, 0x7, 0x8, 0x5e, 0x94, 0xba,

    /* U+0104 "Ą" */
    0x30, 0xc7, 0x14, 0x51, 0x27, 0xa2, 0x8a, 0x20,
    0x82, 0x4,

    /* U+0105 "ą" */
    0x70, 0x41, 0x3c, 0x92, 0x4e, 0x82, 0x8, 0x10,

    /* U+0106 "Ć" */
    0x11, 0x8, 0x7, 0x29, 0x30, 0x84, 0x20, 0x95,
    0x38,

    /* U+0107 "ć" */
    0x11, 0x0, 0x7, 0x29, 0x30, 0x4a, 0x9c,

    /* U+0108 "Ĉ" */
    0x32, 0x94, 0x7, 0x29, 0x30, 0x84, 0x20, 0x95,
    0x38,

    /* U+0109 "ĉ" */
    0x32, 0x80, 0x7, 0x29, 0x30, 0x4a, 0x9c,

    /* U+010A "Ċ" */
    0x20, 0x1c, 0xa4, 0xc2, 0x10, 0x82, 0x54, 0xe0,

    /* U+010B "ċ" */
    0x20, 0x0, 0xe5, 0x26, 0x9, 0x53, 0x80,

    /* U+010C "Č" */
    0x52, 0x8c, 0x7, 0x29, 0x30, 0x84, 0x20, 0x95,
    0x38,

    /* U+010D "č" */
    0x51, 0x80, 0x7, 0x29, 0x30, 0x4a, 0x9c,

    /* U+010E "Ď" */
    0x52, 0x8c, 0xf, 0x4a, 0x51, 0x8c, 0x63, 0x29,
    0x78,

    /* U+010F "ď" */
    0x14, 0x51, 0x5c, 0x92, 0xc9, 0x2c, 0x91, 0xc0,

    /* U+0110 "Đ" */
    0xf2, 0x94, 0x94, 0xf5, 0x2a, 0x57, 0x80,

    /* U+0111 "đ" */
    0x8, 0x4e, 0x10, 0xfe, 0x33, 0x8b, 0x40,

    /* U+0112 "Ē" */
    0xf0, 0xf8, 0x88, 0x8f, 0x88, 0x8f,

    /* U+0113 "ē" */
    0x38, 0x0, 0xe, 0x45, 0x1f, 0xd0, 0x40, 0xe0,

    /* U+0114 "Ĕ" */
    0x88, 0x1c, 0x7, 0xa1, 0x8, 0x43, 0xd0, 0x84,
    0x3c,

    /* U+0115 "ĕ" */
    0x44, 0xe0, 0x0, 0x39, 0x14, 0x7f, 0x41, 0x3,
    0x80,

    /* U+0116 "Ė" */
    0x40, 0xf8, 0x88, 0x8f, 0x88, 0x8f,

    /* U+0117 "ė" */
    0x10, 0x0, 0xe, 0x45, 0x1f, 0xd0, 0x40, 0xe0,

    /* U+0118 "Ę" */
    0xf8, 0x88, 0x8f, 0x88, 0x8f, 0x11, 0x10,

    /* U+0119 "ę" */
    0x39, 0x24, 0x7f, 0x41, 0x3, 0x82, 0x8, 0x10,

    /* U+011A "Ě" */
    0x9a, 0x60, 0xf8, 0x88, 0x8f, 0x88, 0x8f,

    /* U+011B "ě" */
    0x28, 0xc0, 0x0, 0x39, 0x14, 0x7f, 0x41, 0x3,
    0x80,

    /* U+011C "Ĝ" */
    0x31, 0x45, 0x0, 0x71, 0x24, 0x60, 0x82, 0x78,
    0x51, 0x45, 0xe0,

    /* U+011D "ĝ" */
    0x32, 0x80, 0x7, 0xa5, 0x31, 0x8a, 0xda, 0x21,
    0x38,

    /* U+011E "Ğ" */
    0x48, 0x7, 0x0, 0x71, 0x24, 0x60, 0x82, 0x78,
    0x51, 0x45, 0xe0,

    /* U+011F "ğ" */
    0x8b, 0x80, 0x7, 0xa5, 0x31, 0x8a, 0xda, 0x21,
    0x38,

    /* U+0120 "Ġ" */
    0x20, 0x7, 0x12, 0x46, 0x8, 0x27, 0x85, 0x14,
    0x5e,

    /* U+0121 "ġ" */
    0x20, 0x0, 0xf4, 0xa6, 0x31, 0x5b, 0x44, 0x27,
    0x0,

    /* U+0122 "Ģ" */
    0x72, 0x93, 0x8, 0x5e, 0x29, 0x4b, 0x80, 0x2,
    0x10, 0x80,

    /* U+0123 "ģ" */
    0x21, 0x0, 0x7, 0xa5, 0x31, 0x8a, 0xda, 0x21,
    0x38,

    /* U+0124 "Ĥ" */
    0x32, 0x94, 0x8, 0xc6, 0x31, 0x8f, 0xe3, 0x18,
    0xc4,

    /* U+0125 "ĥ" */
    0x32, 0x94, 0x8, 0x42, 0x16, 0x96, 0x63, 0x18,
    0xc4,

    /* U+0126 "Ħ" */
    0x49, 0x2f, 0xd2, 0x49, 0xe4, 0x92, 0x49, 0x20,

    /* U+0127 "ħ" */
    0x42, 0x3c, 0x84, 0x39, 0x29, 0x4a, 0x40,

    /* U+0128 "Ĩ" */
    0x69, 0x2c, 0x7, 0x10, 0x84, 0x21, 0x8, 0x42,
    0x38,

    /* U+0129 "ĩ" */
    0x6d, 0x80, 0x6, 0x10, 0x84, 0x21, 0x1c,

    /* U+012A "Ī" */
    0xf8, 0x1c, 0x42, 0x10, 0x84, 0x21, 0x8, 0xe0,

    /* U+012B "ī" */
    0xf8, 0x0, 0xc2, 0x10, 0x84, 0x27, 0xc0,

    /* U+012C "Ĭ" */
    0x88, 0x1c, 0x7, 0x10, 0x84, 0x21, 0x8, 0x42,
    0x38,

    /* U+012D "ĭ" */
    0x8b, 0x80, 0x7, 0x8, 0x42, 0x10, 0x9e,

    /* U+012E "Į" */
    0xe9, 0x24, 0x92, 0x5d, 0x22,

    /* U+012F "į" */
    0x20, 0x0, 0xc2, 0x10, 0x84, 0x27, 0xc8, 0x41,
    0x0,

    /* U+0130 "İ" */
    0x43, 0xa4, 0x92, 0x49, 0x70,

    /* U+0131 "ı" */
    0x61, 0x8, 0x42, 0x13, 0xe0,

    /* U+0132 "Ĳ" */
    0x8c, 0x63, 0x18, 0xc6, 0x31, 0x8d, 0xc0,

    /* U+0133 "ĳ" */
    0x50, 0xd, 0x55, 0x55, 0x55, 0x11, 0x60,

    /* U+0134 "Ĵ" */
    0x31, 0x8a, 0x3, 0x88, 0x42, 0x10, 0x85, 0x2a,
    0x70,

    /* U+0135 "ĵ" */
    0x69, 0x0, 0xe2, 0x22, 0x22, 0x24, 0x4c,

    /* U+0136 "Ķ" */
    0x8c, 0x63, 0x6a, 0x72, 0xd2, 0x8c, 0x40, 0x2,
    0x10, 0x80,

    /* U+0137 "ķ" */
    0x82, 0x8, 0x26, 0xa2, 0x8e, 0x28, 0xa2, 0x70,
    0x0, 0x20, 0x82, 0x0,

    /* U+0138 "ĸ" */
    0x9e, 0x8a, 0x38, 0xa2, 0x89, 0x80,

    /* U+0139 "Ĺ" */
    0x22, 0x10, 0x8, 0x42, 0x10, 0x84, 0x21, 0x8,
    0x7c,

    /* U+013A "ĺ" */
    0x11, 0x8, 0x6, 0x10, 0x84, 0x21, 0x8, 0x42,
    0x7c,

    /* U+013B "Ļ" */
    0x84, 0x21, 0x8, 0x42, 0x10, 0x87, 0xc0, 0x2,
    0x10, 0x80,

    /* U+013C "ļ" */
    0x61, 0x8, 0x42, 0x10, 0x84, 0x27, 0xc0, 0x2,
    0x10, 0x80,

    /* U+013D "Ľ" */
    0x94, 0xa5, 0x8, 0x42, 0x10, 0x87, 0xc0,

    /* U+013E "ľ" */
    0x69, 0x4a, 0x42, 0x10, 0x84, 0x27, 0xc0,

    /* U+013F "Ŀ" */
    0x84, 0x21, 0x8, 0x4a, 0x10, 0x87, 0xc0,

    /* U+0140 "ŀ" */
    0x61, 0x8, 0x42, 0x14, 0x84, 0x27, 0xc0,

    /* U+0141 "Ł" */
    0x41, 0x4, 0x14, 0x41, 0x8c, 0x10, 0x41, 0xf0,

    /* U+0142 "ł" */
    0xe1, 0x8, 0x72, 0x70, 0x84, 0x27, 0xc0,

    /* U+0143 "Ń" */
    0x11, 0x8, 0x8, 0xc7, 0x3d, 0xad, 0x67, 0x39,
    0xc4,

    /* U+0144 "ń" */
    0x11, 0x0, 0xb, 0xc7, 0x31, 0x8c, 0x62,

    /* U+0145 "Ņ" */
    0x8c, 0x73, 0xde, 0xd6, 0xf7, 0x9c, 0x40, 0x2,
    0x10, 0x80,

    /* U+0146 "ņ" */
    0xbc, 0x73, 0x18, 0xc6, 0x20, 0x1, 0x8, 0x40,

    /* U+0147 "Ň" */
    0x52, 0x8c, 0xc, 0xe7, 0x3d, 0xad, 0x6f, 0x39,
    0xc4,

    /* U+0148 "ň" */
    0x51, 0x80, 0xb, 0x4b, 0x31, 0x8c, 0x62,

    /* U+0149 "ŉ" */
    0xc1, 0x4, 0xa, 0x28, 0xd2, 0x49, 0x24, 0x90,

    /* U+014A "Ŋ" */
    0x8c, 0x73, 0xde, 0xd6, 0xf7, 0x9c, 0x42, 0x13,
    0x80,

    /* U+014B "ŋ" */
    0xbc, 0x73, 0x18, 0xc6, 0x21, 0x8, 0xc0,

    /* U+014C "Ō" */
    0x78, 0x1c, 0xa4, 0xc6, 0x31, 0x8a, 0x52, 0xe0,

    /* U+014D "ō" */
    0x78, 0x0, 0xe4, 0xa6, 0x29, 0x53, 0x80,

    /* U+014E "Ŏ" */
    0x88, 0x1c, 0x7, 0x29, 0x31, 0x8c, 0x62, 0x94,
    0xb8,

    /* U+014F "ŏ" */
    0x8b, 0x80, 0x7, 0x25, 0x31, 0x4a, 0x9c,

    /* U+0150 "Ő" */
    0x2a, 0x94, 0x7, 0x29, 0x31, 0x8c, 0x62, 0x94,
    0xb8,

    /* U+0151 "ő" */
    0x2a, 0x80, 0x7, 0x25, 0x31, 0x4a, 0x9c,

    /* U+0152 "Œ" */
    0x7d, 0x45, 0x24, 0x92, 0x79, 0x14, 0x51, 0xf0,

    /* U+0153 "œ" */
    0x6d, 0xeb, 0x7a, 0x59, 0xa0,

    /* U+0154 "Ŕ" */
    0x11, 0x8, 0xf, 0x4a, 0x72, 0x97, 0xa5, 0x29,
    0x44,

    /* U+0155 "ŕ" */
    0x11, 0x0, 0xb, 0xd5, 0xa8, 0x42, 0x38,

    /* U+0156 "Ŗ" */
    0xf4, 0xa7, 0x29, 0x7a, 0x52, 0x94, 0x40, 0x2,
    0x10, 0x80,

    /* U+0157 "ŗ" */
    0xbd, 0x5a, 0x84, 0x23, 0x80, 0x2, 0x10, 0x80,

    /* U+0158 "Ř" */
    0x52, 0x8c, 0xf, 0x4a, 0x72, 0x97, 0xa5, 0x29,
    0x44,

    /* U+0159 "ř" */
    0x51, 0x80, 0xf, 0xb5, 0xa8, 0x42, 0x38,

    /* U+015A "Ś" */
    0x11, 0x8, 0xf, 0x4a, 0x30, 0x87, 0xc3, 0x19,
    0x78,

    /* U+015B "ś" */
    0x22, 0x0, 0x78, 0x87, 0x11, 0x70,

    /* U+015C "Ŝ" */
    0x32, 0x94, 0x7, 0x4a, 0x30, 0x83, 0xc3, 0x19,
    0x38,

    /* U+015D "ŝ" */
    0x65, 0x0, 0x78, 0x87, 0x11, 0x70,

    /* U+015E "Ş" */
    0xfc, 0x63, 0x8, 0x7c, 0x21, 0x8f, 0xc4, 0x42,
    0x0,

    /* U+015F "ş" */
    0x78, 0x87, 0x11, 0x72, 0x22,

    /* U+0160 "Š" */
    0x52, 0x8c, 0x7, 0x4a, 0x30, 0x83, 0xc3, 0x19,
    0x38,

    /* U+0161 "š" */
    0x56, 0x0, 0x78, 0x87, 0x11, 0x70,

    /* U+0162 "Ţ" */
    0xfc, 0x82, 0x8, 0x20, 0x82, 0x8, 0x20, 0x81,
    0x8, 0x20,

    /* U+0163 "ţ" */
    0x20, 0x82, 0x3f, 0x20, 0x82, 0x8, 0x20, 0x70,
    0x84, 0x10,

    /* U+0164 "Ť" */
    0x51, 0x43, 0x0, 0xfc, 0x82, 0x8, 0x20, 0x82,
    0x8, 0x20, 0x80,

    /* U+0165 "ť" */
    0x9, 0x4a, 0x4f, 0x90, 0x84, 0x21, 0xe,

    /* U+0166 "Ŧ" */
    0xfc, 0x82, 0x8, 0x21, 0xe2, 0x8, 0x20, 0x80,

    /* U+0167 "ŧ" */
    0x21, 0x9, 0xf2, 0x11, 0xc4, 0x21, 0xc0,

    /* U+0168 "Ũ" */
    0x69, 0x2c, 0x8, 0xc6, 0x31, 0x8c, 0x63, 0x18,
    0xb8,

    /* U+0169 "ũ" */
    0x6d, 0x80, 0x8, 0xc6, 0x31, 0xda, 0x5e,

    /* U+016A "Ū" */
    0xf8, 0x23, 0x18, 0xc6, 0x31, 0x8c, 0x63, 0xf0,

    /* U+016B "ū" */
    0x78, 0x1, 0x18, 0xc6, 0x3b, 0x4b, 0xc0,

    /* U+016C "Ŭ" */
    0x88, 0x1c, 0x8, 0xc6, 0x31, 0x8c, 0x63, 0x18,
    0xfc,

    /* U+016D "ŭ" */
    0x8b, 0x80, 0x8, 0xc6, 0x31, 0xda, 0x5e,

    /* U+016E "Ů" */
    0x72, 0x94, 0xe0, 0x46, 0x31, 0x8c, 0x63, 0x18,
    0xc5, 0xc0,

    /* U+016F "ů" */
    0x72, 0x9c, 0x0, 0x46, 0x31, 0x8e, 0xd2, 0xf0,

    /* U+0170 "Ű" */
    0x2a, 0x94, 0x8, 0xc6, 0x31, 0x8c, 0x63, 0x18,
    0xbc,

    /* U+0171 "ű" */
    0x32, 0x80, 0x8, 0xc6, 0x31, 0xda, 0x5e,

    /* U+0172 "Ų" */
    0x8c, 0x63, 0x18, 0xc6, 0x31, 0x8f, 0xc8, 0x41,
    0x0,

    /* U+0173 "ų" */
    0x8a, 0x28, 0xa2, 0xd9, 0x27, 0x82, 0x8, 0x10,

    /* U+0174 "Ŵ" */
    0x23, 0x14, 0xa, 0xd6, 0xb5, 0xad, 0x6b, 0xbd,
    0xa8,

    /* U+0175 "ŵ" */
    0x22, 0x80, 0xa, 0xd6, 0xb5, 0xde, 0xd4,

    /* U+0176 "Ŷ" */
    0x32, 0x94, 0x8, 0xc6, 0x2a, 0x53, 0x88, 0x42,
    0x10,

    /* U+0177 "ŷ" */
    0x32, 0x80, 0x8, 0xc5, 0x2a, 0x52, 0x8c, 0x42,
    0x30,

    /* U+0178 "Ÿ" */
    0x50, 0x23, 0x18, 0xa9, 0x4e, 0x21, 0x8, 0x40,

    /* U+0179 "Ź" */
    0x22, 0x20, 0xf1, 0x11, 0x22, 0x48, 0x8f,

    /* U+017A "ź" */
    0x21, 0x0, 0xf, 0x88, 0x48, 0x84, 0x3e,

    /* U+017B "Ż" */
    0x20, 0xf1, 0x11, 0x22, 0x48, 0x8f,

    /* U+017C "ż" */
    0x20, 0x1, 0xf1, 0x9, 0x10, 0x87, 0xc0,

    /* U+017D "Ž" */
    0x56, 0x60, 0xf1, 0x11, 0x22, 0x48, 0x8f,

    /* U+017E "ž" */
    0x53, 0x0, 0xf, 0x88, 0x48, 0x84, 0x3e
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 102, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 8, .adv_w = 102, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 15, .adv_w = 102, .box_w = 5, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 24, .adv_w = 102, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 31, .adv_w = 102, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 41, .adv_w = 102, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 49, .adv_w = 102, .box_w = 5, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 58, .adv_w = 102, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 65, .adv_w = 102, .box_w = 5, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 74, .adv_w = 102, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 81, .adv_w = 102, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 89, .adv_w = 102, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 96, .adv_w = 102, .box_w = 5, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 105, .adv_w = 102, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 112, .adv_w = 102, .box_w = 5, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 121, .adv_w = 102, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 129, .adv_w = 102, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 136, .adv_w = 102, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 143, .adv_w = 102, .box_w = 4, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 149, .adv_w = 102, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 157, .adv_w = 102, .box_w = 5, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 166, .adv_w = 102, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 175, .adv_w = 102, .box_w = 4, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 181, .adv_w = 102, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 189, .adv_w = 102, .box_w = 4, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 196, .adv_w = 102, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 204, .adv_w = 102, .box_w = 4, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 211, .adv_w = 102, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 220, .adv_w = 102, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 231, .adv_w = 102, .box_w = 5, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 240, .adv_w = 102, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 251, .adv_w = 102, .box_w = 5, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 260, .adv_w = 102, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 269, .adv_w = 102, .box_w = 5, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 278, .adv_w = 102, .box_w = 5, .box_h = 15, .ofs_x = 1, .ofs_y = -5},
    {.bitmap_index = 288, .adv_w = 102, .box_w = 5, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 297, .adv_w = 102, .box_w = 5, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 306, .adv_w = 102, .box_w = 5, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 315, .adv_w = 102, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 323, .adv_w = 102, .box_w = 5, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 330, .adv_w = 102, .box_w = 5, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 339, .adv_w = 102, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 346, .adv_w = 102, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 354, .adv_w = 102, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 361, .adv_w = 102, .box_w = 5, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 370, .adv_w = 102, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 377, .adv_w = 102, .box_w = 3, .box_h = 13, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 382, .adv_w = 102, .box_w = 5, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 391, .adv_w = 102, .box_w = 3, .box_h = 12, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 396, .adv_w = 102, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 401, .adv_w = 102, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 408, .adv_w = 102, .box_w = 4, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 415, .adv_w = 102, .box_w = 5, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 424, .adv_w = 102, .box_w = 4, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 431, .adv_w = 102, .box_w = 5, .box_h = 15, .ofs_x = 1, .ofs_y = -5},
    {.bitmap_index = 441, .adv_w = 102, .box_w = 6, .box_h = 15, .ofs_x = 1, .ofs_y = -5},
    {.bitmap_index = 453, .adv_w = 102, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 459, .adv_w = 102, .box_w = 5, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 468, .adv_w = 102, .box_w = 5, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 477, .adv_w = 102, .box_w = 5, .box_h = 15, .ofs_x = 1, .ofs_y = -5},
    {.bitmap_index = 487, .adv_w = 102, .box_w = 5, .box_h = 15, .ofs_x = 1, .ofs_y = -5},
    {.bitmap_index = 497, .adv_w = 102, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 504, .adv_w = 102, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 511, .adv_w = 102, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 518, .adv_w = 102, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 525, .adv_w = 102, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 533, .adv_w = 102, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 540, .adv_w = 102, .box_w = 5, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 549, .adv_w = 102, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 556, .adv_w = 102, .box_w = 5, .box_h = 15, .ofs_x = 1, .ofs_y = -5},
    {.bitmap_index = 566, .adv_w = 102, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -5},
    {.bitmap_index = 574, .adv_w = 102, .box_w = 5, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 583, .adv_w = 102, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 590, .adv_w = 102, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 598, .adv_w = 102, .box_w = 5, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 607, .adv_w = 102, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 614, .adv_w = 102, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 622, .adv_w = 102, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 629, .adv_w = 102, .box_w = 5, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 638, .adv_w = 102, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 645, .adv_w = 102, .box_w = 5, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 654, .adv_w = 102, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 661, .adv_w = 102, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 669, .adv_w = 102, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 674, .adv_w = 102, .box_w = 5, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 683, .adv_w = 102, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 690, .adv_w = 102, .box_w = 5, .box_h = 15, .ofs_x = 1, .ofs_y = -5},
    {.bitmap_index = 700, .adv_w = 102, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -5},
    {.bitmap_index = 708, .adv_w = 102, .box_w = 5, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 717, .adv_w = 102, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 724, .adv_w = 102, .box_w = 5, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 733, .adv_w = 102, .box_w = 4, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 739, .adv_w = 102, .box_w = 5, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 748, .adv_w = 102, .box_w = 4, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 754, .adv_w = 102, .box_w = 5, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 763, .adv_w = 102, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 768, .adv_w = 102, .box_w = 5, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 777, .adv_w = 102, .box_w = 4, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 783, .adv_w = 102, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 793, .adv_w = 102, .box_w = 6, .box_h = 13, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 803, .adv_w = 102, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 814, .adv_w = 102, .box_w = 5, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 821, .adv_w = 102, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 829, .adv_w = 102, .box_w = 5, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 836, .adv_w = 102, .box_w = 5, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 845, .adv_w = 102, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 852, .adv_w = 102, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 860, .adv_w = 102, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 867, .adv_w = 102, .box_w = 5, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 876, .adv_w = 102, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 883, .adv_w = 102, .box_w = 5, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 893, .adv_w = 102, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 901, .adv_w = 102, .box_w = 5, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 910, .adv_w = 102, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 917, .adv_w = 102, .box_w = 5, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 926, .adv_w = 102, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 934, .adv_w = 102, .box_w = 5, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 943, .adv_w = 102, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 950, .adv_w = 102, .box_w = 5, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 959, .adv_w = 102, .box_w = 5, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 968, .adv_w = 102, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 976, .adv_w = 102, .box_w = 4, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 983, .adv_w = 102, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 990, .adv_w = 102, .box_w = 4, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 996, .adv_w = 102, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1003, .adv_w = 102, .box_w = 4, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1010, .adv_w = 102, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0}
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
const lv_font_t pda_font_fallback_16 = {
#else
lv_font_t pda_font_fallback_16 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 20,          /*The maximum line height required by the font*/
    .base_line = 5,             /*Baseline measured from the bottom of the line*/
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



#endif /*#if PDA_FONT_FALLBACK_16*/

