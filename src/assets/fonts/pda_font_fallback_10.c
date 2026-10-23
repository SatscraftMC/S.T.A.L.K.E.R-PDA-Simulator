/*******************************************************************************
 * Size: 10 px
 * Bpp: 1
 * Opts: --font /usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf --size 10 --bpp 1 --format lvgl --lv-include lvgl.h --no-compress -r 0x100-0x17F -r 0x0400-0x04FF --lv-font-name pda_font_fallback_10 -o src/assets/fonts/pda_font_fallback_10.c
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
    0x30, 0x3, 0xc, 0x31, 0x24, 0x9e, 0x4a, 0x10,

    /* U+0101 "ā" */
    0x70, 0x3c, 0x17, 0xc6, 0x2f,

    /* U+0102 "Ă" */
    0x30, 0x3, 0xc, 0x31, 0x24, 0x9e, 0x4a, 0x10,

    /* U+0103 "ă" */
    0x43, 0x81, 0xe0, 0xbe, 0x31, 0x78,

    /* U+0104 "Ą" */
    0x20, 0xc5, 0x14, 0x49, 0xe8, 0xa2, 0x8, 0x30,

    /* U+0105 "ą" */
    0xf0, 0x5f, 0x18, 0xbc, 0x43,

    /* U+0106 "Ć" */
    0x11, 0x1f, 0x98, 0x42, 0x10, 0xcb, 0xc0,

    /* U+0107 "ć" */
    0x12, 0x7, 0x88, 0x88, 0x70,

    /* U+0108 "Ĉ" */
    0x30, 0x1f, 0x98, 0x42, 0x10, 0xcb, 0xc0,

    /* U+0109 "ĉ" */
    0x26, 0x7, 0x88, 0x88, 0x70,

    /* U+010A "Ċ" */
    0x20, 0x1f, 0x98, 0x42, 0x10, 0xcb, 0xc0,

    /* U+010B "ċ" */
    0x20, 0x78, 0x88, 0x87,

    /* U+010C "Č" */
    0x51, 0x1f, 0x98, 0x42, 0x10, 0xcb, 0xc0,

    /* U+010D "č" */
    0x52, 0x7, 0x88, 0x88, 0x70,

    /* U+010E "Ď" */
    0x61, 0x3d, 0x28, 0xc6, 0x31, 0x9f, 0x80,

    /* U+010F "ď" */
    0xc, 0x37, 0xa2, 0x8a, 0x28, 0x9e,

    /* U+0110 "Đ" */
    0x79, 0x24, 0x79, 0x45, 0x14, 0xde,

    /* U+0111 "đ" */
    0x3c, 0x27, 0xa2, 0x8a, 0x28, 0x9e,

    /* U+0112 "Ē" */
    0x70, 0xf8, 0x8f, 0x88, 0x8f,

    /* U+0113 "ē" */
    0x70, 0x1d, 0x1f, 0xc3, 0xf,

    /* U+0114 "Ĕ" */
    0x60, 0xf8, 0x8f, 0x88, 0x8f,

    /* U+0115 "ĕ" */
    0x53, 0x80, 0xe8, 0xfe, 0x18, 0x78,

    /* U+0116 "Ė" */
    0x20, 0x3f, 0x8, 0x7e, 0x10, 0x87, 0xc0,

    /* U+0117 "ė" */
    0x20, 0x1d, 0x1f, 0xc3, 0xf,

    /* U+0118 "Ę" */
    0xf8, 0x8f, 0x88, 0x8f, 0x21,

    /* U+0119 "ę" */
    0x74, 0x7f, 0xc, 0x3c, 0x83,

    /* U+011A "Ě" */
    0x62, 0xf8, 0x8f, 0x88, 0x8f,

    /* U+011B "ě" */
    0x42, 0x88, 0xe8, 0xfe, 0x18, 0x78,

    /* U+011C "Ĝ" */
    0x22, 0x9e, 0x98, 0x42, 0x71, 0xcb, 0xc0,

    /* U+011D "ĝ" */
    0x22, 0x80, 0xf8, 0xc6, 0x31, 0x78, 0x5c,

    /* U+011E "Ğ" */
    0x70, 0x1e, 0x98, 0x42, 0x71, 0xcb, 0xc0,

    /* U+011F "ğ" */
    0x43, 0x80, 0xf8, 0xc6, 0x31, 0x78, 0x5c,

    /* U+0120 "Ġ" */
    0x20, 0x1e, 0x98, 0x42, 0x71, 0xcb, 0xc0,

    /* U+0121 "ġ" */
    0x20, 0x1f, 0x18, 0xc6, 0x2f, 0xb, 0x80,

    /* U+0122 "Ģ" */
    0x7a, 0x61, 0x9, 0xc7, 0x2f, 0x1, 0x88,

    /* U+0123 "ģ" */
    0x11, 0x0, 0xf8, 0xc6, 0x31, 0x78, 0x5c,

    /* U+0124 "Ĥ" */
    0x22, 0xa3, 0x18, 0xfe, 0x31, 0x8c, 0x40,

    /* U+0125 "ĥ" */
    0x60, 0x88, 0xf9, 0x99, 0x99,

    /* U+0126 "Ħ" */
    0x45, 0xfd, 0x13, 0xe4, 0x48, 0x91, 0x22,

    /* U+0127 "ħ" */
    0xe2, 0x1e, 0x94, 0xa5, 0x29,

    /* U+0128 "Ĩ" */
    0x70, 0x3e, 0x42, 0x10, 0x84, 0x27, 0xc0,

    /* U+0129 "ĩ" */
    0x70, 0x18, 0x42, 0x10, 0x9f,

    /* U+012A "Ī" */
    0x70, 0x3e, 0x42, 0x10, 0x84, 0x27, 0xc0,

    /* U+012B "ī" */
    0x70, 0x18, 0x42, 0x10, 0x9f,

    /* U+012C "Ĭ" */
    0x70, 0x3e, 0x42, 0x10, 0x84, 0x27, 0xc0,

    /* U+012D "ĭ" */
    0x13, 0x80, 0xc2, 0x10, 0x84, 0xf8,

    /* U+012E "Į" */
    0xf9, 0x8, 0x42, 0x10, 0x9f, 0x21, 0x80,

    /* U+012F "į" */
    0x20, 0x18, 0x42, 0x10, 0x9f, 0x21, 0x80,

    /* U+0130 "İ" */
    0x20, 0x3e, 0x42, 0x10, 0x84, 0x27, 0xc0,

    /* U+0131 "ı" */
    0x61, 0x8, 0x42, 0x7c,

    /* U+0132 "Ĳ" */
    0xed, 0x14, 0x51, 0x45, 0x14, 0x7f,

    /* U+0133 "ĳ" */
    0x48, 0x36, 0x94, 0xa5, 0x3d, 0x9, 0xc0,

    /* U+0134 "Ĵ" */
    0x25, 0x71, 0x11, 0x11, 0x1e,

    /* U+0135 "ĵ" */
    0x66, 0xe, 0x22, 0x22, 0x22, 0xe0,

    /* U+0136 "Ķ" */
    0x8c, 0xa9, 0xca, 0x4a, 0x51, 0x1, 0x8,

    /* U+0137 "ķ" */
    0x84, 0x25, 0x4c, 0x52, 0x52, 0x1, 0x8,

    /* U+0138 "ĸ" */
    0x95, 0x31, 0x49, 0x48,

    /* U+0139 "Ĺ" */
    0x84, 0x21, 0x8, 0x42, 0x10, 0x87, 0xc0,

    /* U+013A "ĺ" */
    0x40, 0xc4, 0x44, 0x44, 0x47,

    /* U+013B "Ļ" */
    0x84, 0x21, 0x8, 0x42, 0x1f, 0x1, 0x10,

    /* U+013C "ļ" */
    0xc4, 0x44, 0x44, 0x47, 0x4, 0x40,

    /* U+013D "Ľ" */
    0xa5, 0x21, 0x8, 0x42, 0x1f,

    /* U+013E "ľ" */
    0xd2, 0x90, 0x84, 0x21, 0xe,

    /* U+013F "Ŀ" */
    0x88, 0x89, 0x88, 0x8f,

    /* U+0140 "ŀ" */
    0xc2, 0x10, 0x94, 0x21, 0x6,

    /* U+0141 "Ł" */
    0x41, 0x5, 0x18, 0xc1, 0x4, 0x1f,

    /* U+0142 "ł" */
    0x61, 0xa, 0x66, 0x10, 0x87,

    /* U+0143 "Ń" */
    0x11, 0x33, 0x9c, 0xd6, 0xb3, 0x9c, 0xc0,

    /* U+0144 "ń" */
    0x12, 0x4f, 0x99, 0x99, 0x90,

    /* U+0145 "Ņ" */
    0xce, 0x73, 0x5a, 0xce, 0x73, 0x1, 0x8,

    /* U+0146 "ņ" */
    0xf9, 0x99, 0x99, 0x2, 0x40,

    /* U+0147 "Ň" */
    0x51, 0x33, 0x9c, 0xd6, 0xb3, 0x9c, 0xc0,

    /* U+0148 "ň" */
    0x86, 0xf, 0x99, 0x99, 0x90,

    /* U+0149 "ŉ" */
    0x84, 0x3e, 0x94, 0xa5, 0x29,

    /* U+014A "Ŋ" */
    0xb6, 0x63, 0x18, 0xc6, 0x31, 0x9, 0x80,

    /* U+014B "ŋ" */
    0xf9, 0x99, 0x99, 0x17,

    /* U+014C "Ō" */
    0x70, 0x1d, 0x18, 0xc6, 0x31, 0x8b, 0x80,

    /* U+014D "ō" */
    0x70, 0x1d, 0x18, 0xc6, 0x2e,

    /* U+014E "Ŏ" */
    0x70, 0x1d, 0x18, 0xc6, 0x31, 0x8b, 0x80,

    /* U+014F "ŏ" */
    0x3, 0x80, 0xe8, 0xc6, 0x31, 0x70,

    /* U+0150 "Ő" */
    0x50, 0x1d, 0x18, 0xc6, 0x31, 0x8b, 0x80,

    /* U+0151 "ő" */
    0x32, 0x80, 0xe8, 0xc6, 0x31, 0x70,

    /* U+0152 "Œ" */
    0x7e, 0x49, 0x27, 0x92, 0x49, 0x1f,

    /* U+0153 "œ" */
    0x6e, 0x59, 0xe4, 0x91, 0xb0,

    /* U+0154 "Ŕ" */
    0x42, 0x39, 0x29, 0x4b, 0x92, 0x94, 0x40,

    /* U+0155 "ŕ" */
    0x12, 0xb, 0xc8, 0x88, 0x80,

    /* U+0156 "Ŗ" */
    0xe4, 0xa5, 0x2e, 0x4a, 0x51, 0x1, 0x10,

    /* U+0157 "ŗ" */
    0xbc, 0x88, 0x88, 0x8, 0x80,

    /* U+0158 "Ř" */
    0x62, 0x39, 0x29, 0x4b, 0x92, 0x94, 0x40,

    /* U+0159 "ř" */
    0x86, 0x4b, 0xc8, 0x88, 0x80,

    /* U+015A "Ś" */
    0x21, 0x1f, 0x18, 0x70, 0x61, 0xf, 0x80,

    /* U+015B "ś" */
    0x22, 0xf, 0x8c, 0x31, 0xe0,

    /* U+015C "Ŝ" */
    0x22, 0x9f, 0x18, 0x70, 0x61, 0xf, 0x80,

    /* U+015D "ŝ" */
    0x6, 0xf, 0x8c, 0x31, 0xe0,

    /* U+015E "Ş" */
    0x7c, 0x61, 0xc1, 0x84, 0x3e, 0x23, 0x0,

    /* U+015F "ş" */
    0xf8, 0xc3, 0x1e, 0x26,

    /* U+0160 "Š" */
    0x51, 0x1f, 0x18, 0x70, 0x61, 0xf, 0x80,

    /* U+0161 "š" */
    0x96, 0xf, 0x8c, 0x31, 0xe0,

    /* U+0162 "Ţ" */
    0xf9, 0x8, 0x42, 0x10, 0x84, 0x23, 0x0,

    /* U+0163 "ţ" */
    0x44, 0xf4, 0x44, 0x47, 0x26,

    /* U+0164 "Ť" */
    0x51, 0x3e, 0x42, 0x10, 0x84, 0x21, 0x0,

    /* U+0165 "ť" */
    0x15, 0x4f, 0x44, 0x44, 0x70,

    /* U+0166 "Ŧ" */
    0xf9, 0x8, 0x47, 0x10, 0x84,

    /* U+0167 "ŧ" */
    0x44, 0xf4, 0xe4, 0x47,

    /* U+0168 "Ũ" */
    0x70, 0x23, 0x18, 0xc6, 0x31, 0x8b, 0x80,

    /* U+0169 "ũ" */
    0x60, 0x99, 0x99, 0x9f,

    /* U+016A "Ū" */
    0x70, 0x23, 0x18, 0xc6, 0x31, 0x8b, 0x80,

    /* U+016B "ū" */
    0x60, 0x99, 0x99, 0x9f,

    /* U+016C "Ŭ" */
    0x70, 0x23, 0x18, 0xc6, 0x31, 0x8b, 0x80,

    /* U+016D "ŭ" */
    0x6, 0x9, 0x99, 0x99, 0xf0,

    /* U+016E "Ů" */
    0x72, 0xbf, 0x18, 0xc6, 0x31, 0x8b, 0x80,

    /* U+016F "ů" */
    0x75, 0x79, 0x99, 0x99, 0xf0,

    /* U+0170 "Ű" */
    0x50, 0x23, 0x18, 0xc6, 0x31, 0x8b, 0x80,

    /* U+0171 "ű" */
    0x56, 0x9, 0x99, 0x99, 0xf0,

    /* U+0172 "Ų" */
    0x8c, 0x63, 0x18, 0xc6, 0x2e, 0x41, 0x80,

    /* U+0173 "ų" */
    0x94, 0xa5, 0x29, 0x78, 0x43,

    /* U+0174 "Ŵ" */
    0x30, 0x28, 0x61, 0xb6, 0xdb, 0x9e, 0x49, 0x20,

    /* U+0175 "ŵ" */
    0x0, 0xc4, 0xa1, 0x86, 0xe7, 0x9e, 0x48,

    /* U+0176 "Ŷ" */
    0x22, 0xa3, 0x15, 0x30, 0x84, 0x21, 0x0,

    /* U+0177 "ŷ" */
    0x63, 0x1, 0x29, 0x49, 0x8c, 0x22, 0x30,

    /* U+0178 "Ÿ" */
    0x50, 0x23, 0x15, 0x28, 0x84, 0x21, 0x0,

    /* U+0179 "Ź" */
    0x21, 0x3e, 0x11, 0x10, 0x88, 0x87, 0xc0,

    /* U+017A "ź" */
    0x12, 0xf, 0x12, 0x48, 0xf0,

    /* U+017B "Ż" */
    0x20, 0x3e, 0x31, 0x10, 0x88, 0x87, 0xc0,

    /* U+017C "ż" */
    0x20, 0x1e, 0x22, 0x11, 0x1f,

    /* U+017D "Ž" */
    0x51, 0x3e, 0x11, 0x10, 0x88, 0x87, 0xc0,

    /* U+017E "ž" */
    0x96, 0xf, 0x12, 0x48, 0xf0,

    /* U+017F "ſ" */
    0x74, 0xc4, 0x44, 0x44,

    /* U+0400 "Ѐ" */
    0x42, 0xf8, 0x8f, 0x88, 0x8f,

    /* U+0401 "Ё" */
    0x50, 0x3f, 0x8, 0x7a, 0x10, 0x87, 0xc0,

    /* U+0402 "Ђ" */
    0xf1, 0x4, 0x1f, 0x45, 0x14, 0x51, 0x4, 0x60,

    /* U+0403 "Ѓ" */
    0x20, 0x3f, 0x8, 0x42, 0x10, 0x84, 0x0,

    /* U+0404 "Є" */
    0x7e, 0x21, 0xe8, 0x43, 0x2f,

    /* U+0405 "Ѕ" */
    0x7c, 0x61, 0xc1, 0x84, 0x3e,

    /* U+0406 "І" */
    0xf9, 0x8, 0x42, 0x10, 0x9f,

    /* U+0407 "Ї" */
    0x50, 0x3e, 0x42, 0x10, 0x84, 0x27, 0xc0,

    /* U+0408 "Ј" */
    0x71, 0x11, 0x11, 0x1e,

    /* U+0409 "Љ" */
    0x71, 0x45, 0x16, 0x55, 0x55, 0x66,

    /* U+040A "Њ" */
    0x92, 0x49, 0x3e, 0x96, 0x59, 0x66,

    /* U+040B "Ћ" */
    0xf1, 0x4, 0x1e, 0x45, 0x14, 0x51,

    /* U+040C "Ќ" */
    0x21, 0x23, 0x2a, 0x72, 0x92, 0x94, 0x40,

    /* U+040D "Ѝ" */
    0x21, 0x27, 0x39, 0xd6, 0xb9, 0xce, 0x40,

    /* U+040E "Ў" */
    0x60, 0x25, 0x29, 0x31, 0x8c, 0x46, 0x0,

    /* U+040F "Џ" */
    0x8c, 0x63, 0x18, 0xc6, 0x3f, 0x21, 0x0,

    /* U+0410 "А" */
    0x30, 0xc3, 0x12, 0x49, 0xe4, 0xa1,

    /* U+0411 "Б" */
    0xfc, 0x21, 0xe8, 0xc6, 0x3e,

    /* U+0412 "В" */
    0xe4, 0xa5, 0xc9, 0xc6, 0x3e,

    /* U+0413 "Г" */
    0xfc, 0x21, 0x8, 0x42, 0x10,

    /* U+0414 "Д" */
    0x79, 0x24, 0x92, 0x49, 0x24, 0xbf, 0x84,

    /* U+0415 "Е" */
    0xf8, 0x8f, 0x88, 0x8f,

    /* U+0416 "Ж" */
    0xad, 0x5c, 0xe7, 0x56, 0xb5,

    /* U+0417 "З" */
    0x70, 0x42, 0x60, 0x86, 0x3e,

    /* U+0418 "И" */
    0x9c, 0xe7, 0x5a, 0xe7, 0x39,

    /* U+0419 "Й" */
    0x70, 0x27, 0x39, 0xd6, 0xb9, 0xce, 0x40,

    /* U+041A "К" */
    0x8c, 0xa9, 0xca, 0x4a, 0x51,

    /* U+041B "Л" */
    0x7a, 0x52, 0x94, 0xa5, 0x31,

    /* U+041C "М" */
    0xde, 0xf7, 0xda, 0xc6, 0x31,

    /* U+041D "Н" */
    0x8c, 0x63, 0xf8, 0xc6, 0x31,

    /* U+041E "О" */
    0x74, 0x63, 0x18, 0xc6, 0x2e,

    /* U+041F "П" */
    0xfc, 0x63, 0x18, 0xc6, 0x31,

    /* U+0420 "Р" */
    0xf4, 0x63, 0x1f, 0x42, 0x10,

    /* U+0421 "С" */
    0x7e, 0x61, 0x8, 0x43, 0x2f,

    /* U+0422 "Т" */
    0xf9, 0x8, 0x42, 0x10, 0x84,

    /* U+0423 "У" */
    0x94, 0xa4, 0xc6, 0x31, 0x18,

    /* U+0424 "Ф" */
    0x23, 0xab, 0x5a, 0xd5, 0xc4,

    /* U+0425 "Х" */
    0x49, 0x23, 0xc, 0x30, 0xa4, 0xa1,

    /* U+0426 "Ц" */
    0x8c, 0x63, 0x18, 0xc6, 0x3f, 0x8,

    /* U+0427 "Ч" */
    0x8c, 0x63, 0x1f, 0x84, 0x21,

    /* U+0428 "Ш" */
    0xad, 0x6b, 0x5a, 0xd6, 0xbf,

    /* U+0429 "Щ" */
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xbf, 0x4,

    /* U+042A "Ъ" */
    0xc2, 0x10, 0xe4, 0xa5, 0x2e,

    /* U+042B "Ы" */
    0x8c, 0x63, 0xd9, 0xce, 0x7d,

    /* U+042C "Ь" */
    0x84, 0x21, 0xe8, 0xc6, 0x3e,

    /* U+042D "Э" */
    0xf0, 0xc2, 0xf0, 0x86, 0x7e,

    /* U+042E "Ю" */
    0xb5, 0x6b, 0xda, 0xd6, 0xb2,

    /* U+042F "Я" */
    0x7c, 0x63, 0x17, 0xa7, 0x31,

    /* U+0430 "а" */
    0xf0, 0x5f, 0x18, 0xbc,

    /* U+0431 "б" */
    0x7e, 0x3d, 0x18, 0xc6, 0x2e,

    /* U+0432 "в" */
    0xf9, 0xe9, 0x9e,

    /* U+0433 "г" */
    0xf8, 0x88, 0x88,

    /* U+0434 "д" */
    0x72, 0x94, 0xa5, 0x7e, 0x20,

    /* U+0435 "е" */
    0x74, 0x7f, 0xc, 0x3c,

    /* U+0436 "ж" */
    0xab, 0x9d, 0x6a, 0xd4,

    /* U+0437 "з" */
    0xe1, 0x61, 0x1e,

    /* U+0438 "и" */
    0x9b, 0xbd, 0xd9,

    /* U+0439 "й" */
    0x6, 0x9, 0xbb, 0xdd, 0x90,

    /* U+043A "к" */
    0x95, 0x31, 0x49, 0x48,

    /* U+043B "л" */
    0x7a, 0x52, 0x94, 0xc4,

    /* U+043C "м" */
    0x87, 0x3c, 0xed, 0xb6, 0x10,

    /* U+043D "н" */
    0x99, 0xf9, 0x99,

    /* U+043E "о" */
    0x74, 0x63, 0x18, 0xb8,

    /* U+043F "п" */
    0xf9, 0x99, 0x99,

    /* U+0440 "р" */
    0xf4, 0x63, 0x18, 0xfa, 0x10,

    /* U+0441 "с" */
    0x78, 0x88, 0x87,

    /* U+0442 "т" */
    0xf9, 0x8, 0x42, 0x10,

    /* U+0443 "у" */
    0x94, 0xa4, 0xc6, 0x11, 0x18,

    /* U+0444 "ф" */
    0x21, 0x9, 0xfa, 0xd6, 0xb5, 0x71, 0x8,

    /* U+0445 "х" */
    0x48, 0xc3, 0xc, 0x49, 0x20,

    /* U+0446 "ц" */
    0x94, 0xa5, 0x29, 0x7c, 0x20,

    /* U+0447 "ч" */
    0x99, 0x9f, 0x11,

    /* U+0448 "ш" */
    0xad, 0x6b, 0x5a, 0xfc,

    /* U+0449 "щ" */
    0xaa, 0xaa, 0xaa, 0xab, 0xf0, 0x40,

    /* U+044A "ъ" */
    0xc2, 0x1c, 0x94, 0xb8,

    /* U+044B "ы" */
    0x8c, 0x7b, 0x39, 0xf4,

    /* U+044C "ь" */
    0x88, 0xe9, 0x9e,

    /* U+044D "э" */
    0xe1, 0x71, 0x1e,

    /* U+044E "ю" */
    0xb5, 0x7b, 0x5a, 0xc8,

    /* U+044F "я" */
    0x79, 0x97, 0x59,

    /* U+0450 "ѐ" */
    0x41, 0x0, 0xe8, 0xfe, 0x18, 0x78,

    /* U+0451 "ё" */
    0x50, 0x1d, 0x1f, 0xc3, 0xf,

    /* U+0452 "ђ" */
    0x42, 0x3c, 0x87, 0xa5, 0x29, 0x8, 0x80,

    /* U+0453 "ѓ" */
    0x12, 0xf, 0x88, 0x88, 0x80,

    /* U+0454 "є" */
    0x78, 0xe8, 0x87,

    /* U+0455 "ѕ" */
    0xf8, 0xc3, 0x1e,

    /* U+0456 "і" */
    0x20, 0x18, 0x42, 0x10, 0x9f,

    /* U+0457 "ї" */
    0x50, 0x18, 0x42, 0x10, 0x9f,

    /* U+0458 "ј" */
    0x23, 0x92, 0x49, 0x3c,

    /* U+0459 "љ" */
    0x71, 0x45, 0x95, 0x56, 0x60,

    /* U+045A "њ" */
    0x94, 0xbf, 0x39, 0xcc,

    /* U+045B "ћ" */
    0x42, 0x3c, 0x87, 0xa5, 0x29,

    /* U+045C "ќ" */
    0x11, 0x11, 0x2a, 0x62, 0x92, 0x90,

    /* U+045D "ѝ" */
    0x44, 0x29, 0xbb, 0xdd, 0x90,

    /* U+045E "ў" */
    0x93, 0x1, 0x29, 0x49, 0x8c, 0x22, 0x30,

    /* U+045F "џ" */
    0x99, 0x99, 0x9f, 0x22,

    /* U+0462 "Ѣ" */
    0x47, 0x90, 0xe4, 0xa5, 0x2e,

    /* U+0463 "ѣ" */
    0x42, 0x3c, 0x87, 0x25, 0x2e,

    /* U+0472 "Ѳ" */
    0x74, 0x63, 0xf8, 0xc7, 0x6e,

    /* U+0473 "ѳ" */
    0x74, 0x63, 0xf8, 0xb8,

    /* U+0490 "Ґ" */
    0x8, 0x7f, 0x8, 0x42, 0x10, 0x84, 0x0,

    /* U+0491 "ґ" */
    0x11, 0xf8, 0x88, 0x88,

    /* U+0492 "Ғ" */
    0x7d, 0x4, 0x3e, 0x41, 0x4, 0x10,

    /* U+0493 "ғ" */
    0x7a, 0x3c, 0x84, 0x20,

    /* U+0494 "Ҕ" */
    0xfc, 0x21, 0xe8, 0xc6, 0x31, 0x9, 0x80,

    /* U+0495 "ҕ" */
    0xe8, 0xf9, 0x99, 0x13,

    /* U+0496 "Җ" */
    0xad, 0x9c, 0xe7, 0x5a, 0xb5, 0x8,

    /* U+0497 "җ" */
    0xb3, 0x9d, 0x6b, 0x54, 0x20,

    /* U+0498 "Ҙ" */
    0xe1, 0x16, 0x11, 0x1e, 0x4c,

    /* U+0499 "ҙ" */
    0xe1, 0x61, 0x1e, 0x4c,

    /* U+049A "Қ" */
    0x9c, 0xa9, 0x8a, 0x4a, 0x51, 0x8,

    /* U+049B "қ" */
    0x95, 0x39, 0x49, 0x4c, 0x20,

    /* U+04A2 "Ң" */
    0x8a, 0x28, 0xbe, 0x8a, 0x28, 0xa3, 0x4,

    /* U+04A3 "ң" */
    0x94, 0xbd, 0x29, 0x4c, 0x20,

    /* U+04A4 "Ҥ" */
    0x9e, 0x49, 0x3c, 0x92, 0x49, 0x24,

    /* U+04A5 "ҥ" */
    0x9e, 0x4f, 0x24, 0x92, 0x40,

    /* U+04AA "Ҫ" */
    0x7e, 0x61, 0x8, 0x43, 0x2e, 0x11, 0x80,

    /* U+04AB "ҫ" */
    0x78, 0x88, 0x87, 0x22,

    /* U+04AC "Ҭ" */
    0xf9, 0x8, 0x42, 0x10, 0x86, 0x10,

    /* U+04AD "ҭ" */
    0xf9, 0x8, 0x42, 0x18, 0x40,

    /* U+04AE "Ү" */
    0x8c, 0x54, 0xc2, 0x10, 0x84,

    /* U+04AF "ү" */
    0x8c, 0x54, 0xa6, 0x10, 0x84,

    /* U+04B0 "Ұ" */
    0x8c, 0x54, 0xa2, 0x38, 0x84,

    /* U+04B1 "ұ" */
    0x8c, 0x54, 0xa6, 0x11, 0xc4,

    /* U+04B2 "Ҳ" */
    0x45, 0x23, 0x4, 0x30, 0xa4, 0xb1, 0x4,

    /* U+04B3 "ҳ" */
    0x92, 0x88, 0xc5, 0x44, 0x20,

    /* U+04BA "Һ" */
    0x84, 0x21, 0xe8, 0xc6, 0x31,

    /* U+04BB "һ" */
    0x88, 0xf9, 0x99, 0x99,

    /* U+04C0 "Ӏ" */
    0xf9, 0x8, 0x42, 0x10, 0x9f,

    /* U+04C1 "Ӂ" */
    0x70, 0x2b, 0x57, 0x39, 0xd5, 0xad, 0x40,

    /* U+04C2 "ӂ" */
    0x3, 0x81, 0x57, 0x3a, 0xd5, 0xa8,

    /* U+04C3 "Ӄ" */
    0x8a, 0x4a, 0x3c, 0x8a, 0x28, 0xa2, 0x8, 0xc0,

    /* U+04C4 "ӄ" */
    0xbc, 0xf9, 0x99, 0x13,

    /* U+04C7 "Ӈ" */
    0x8c, 0x63, 0xf8, 0xc6, 0x31, 0x9, 0x80,

    /* U+04C8 "ӈ" */
    0x99, 0xf9, 0x99, 0x13,

    /* U+04CB "Ӌ" */
    0x8c, 0x63, 0x17, 0x84, 0x23, 0x10,

    /* U+04CC "ӌ" */
    0x99, 0x9f, 0x13, 0x20,

    /* U+04CF "ӏ" */
    0xff,

    /* U+04D0 "Ӑ" */
    0x30, 0x3, 0xc, 0x31, 0x24, 0x9e, 0x4a, 0x10,

    /* U+04D1 "ӑ" */
    0x43, 0x81, 0xe0, 0xbe, 0x31, 0x78,

    /* U+04D2 "Ӓ" */
    0x50, 0x8, 0xa5, 0x29, 0x5f, 0x8c, 0x40,

    /* U+04D3 "ӓ" */
    0x50, 0x3c, 0x17, 0xc6, 0x2f,

    /* U+04D4 "Ӕ" */
    0x3d, 0x45, 0x17, 0x51, 0xc9, 0x27,

    /* U+04D5 "ӕ" */
    0xd9, 0x7f, 0x4a, 0x6c,

    /* U+04D6 "Ӗ" */
    0x60, 0xf8, 0x8f, 0x88, 0x8f,

    /* U+04D7 "ӗ" */
    0x53, 0x80, 0xe8, 0xfe, 0x18, 0x78,

    /* U+04D8 "Ә" */
    0x70, 0x43, 0xf8, 0xc7, 0x6e,

    /* U+04D9 "ә" */
    0xf0, 0xc3, 0xf8, 0xb8,

    /* U+04DA "Ӛ" */
    0x50, 0x1c, 0x10, 0xfe, 0x31, 0xdb, 0x80,

    /* U+04DB "ӛ" */
    0x50, 0x3c, 0x30, 0xfe, 0x2e,

    /* U+04DC "Ӝ" */
    0x50, 0xa, 0xaa, 0x71, 0xc7, 0x2a, 0xaa, 0xa0,

    /* U+04DD "ӝ" */
    0x50, 0x2a, 0xe7, 0x5a, 0xb5,

    /* U+04DE "Ӟ" */
    0x50, 0x3c, 0x10, 0xb8, 0x21, 0xf, 0x80,

    /* U+04DF "ӟ" */
    0x50, 0x1c, 0x17, 0x4, 0x3e,

    /* U+04E0 "Ӡ" */
    0xf8, 0x21, 0xe, 0x6, 0x18, 0x5e,

    /* U+04E1 "ӡ" */
    0xf8, 0x88, 0xe0, 0x84, 0x3e,

    /* U+04E2 "Ӣ" */
    0x70, 0x27, 0x39, 0xd6, 0xb9, 0xce, 0x40,

    /* U+04E3 "ӣ" */
    0x60, 0x9b, 0xbd, 0xd9,

    /* U+04E4 "Ӥ" */
    0x50, 0x27, 0x39, 0xd6, 0xb9, 0xce, 0x40,

    /* U+04E5 "ӥ" */
    0x50, 0x9b, 0xbd, 0xd9,

    /* U+04E6 "Ӧ" */
    0x50, 0x1d, 0x18, 0xc6, 0x31, 0x8b, 0x80,

    /* U+04E7 "ӧ" */
    0x50, 0x1d, 0x18, 0xc6, 0x2e,

    /* U+04E8 "Ө" */
    0x74, 0x63, 0xf8, 0xc7, 0x6e,

    /* U+04E9 "ө" */
    0x74, 0x63, 0xf8, 0xb8,

    /* U+04EA "Ӫ" */
    0x50, 0x1d, 0x18, 0xfe, 0x31, 0xdb, 0x80,

    /* U+04EB "ӫ" */
    0x50, 0x1d, 0x18, 0xfe, 0x2e,

    /* U+04EC "Ӭ" */
    0x50, 0x3c, 0x30, 0xbc, 0x21, 0x9f, 0x80,

    /* U+04ED "ӭ" */
    0x50, 0x1c, 0x17, 0x86, 0x6e,

    /* U+04EE "Ӯ" */
    0x60, 0x25, 0x29, 0x31, 0x8c, 0x46, 0x0,

    /* U+04EF "ӯ" */
    0x60, 0x25, 0x29, 0x31, 0x84, 0x46, 0x0,

    /* U+04F0 "Ӱ" */
    0x50, 0x23, 0x95, 0x28, 0xc4, 0x26, 0x0,

    /* U+04F1 "ӱ" */
    0x50, 0x23, 0x15, 0x28, 0xc4, 0x26, 0x0,

    /* U+04F2 "Ӳ" */
    0x50, 0x25, 0x29, 0x31, 0x8c, 0x46, 0x0,

    /* U+04F3 "ӳ" */
    0x53, 0x1, 0x29, 0x49, 0x8c, 0x22, 0x30,

    /* U+04F4 "Ӵ" */
    0x50, 0x23, 0x18, 0xc7, 0xe1, 0x8, 0x40,

    /* U+04F5 "ӵ" */
    0x50, 0x99, 0x9f, 0x11,

    /* U+04F6 "Ӷ" */
    0xf8, 0x88, 0x88, 0x8c, 0x40,

    /* U+04F7 "ӷ" */
    0xf8, 0x88, 0x8c, 0x40,

    /* U+04F8 "Ӹ" */
    0x50, 0x23, 0x18, 0xf6, 0x73, 0x9f, 0x40,

    /* U+04F9 "ӹ" */
    0x50, 0x23, 0x1e, 0xce, 0x7d
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 96, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 8, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 13, .adv_w = 96, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 21, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 27, .adv_w = 96, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 35, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 40, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 47, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 52, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 59, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 64, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 71, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 75, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 82, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 87, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 94, .adv_w = 96, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 100, .adv_w = 96, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 106, .adv_w = 96, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 112, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 117, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 122, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 127, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 133, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 140, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 145, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 150, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 155, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 160, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 166, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 173, .adv_w = 96, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 180, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 187, .adv_w = 96, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 194, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 201, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 208, .adv_w = 96, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 215, .adv_w = 96, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 222, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 229, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 234, .adv_w = 96, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 241, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 246, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 253, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 258, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 265, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 270, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 277, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 283, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 290, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 297, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 304, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 308, .adv_w = 96, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 314, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 321, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 326, .adv_w = 96, .box_w = 4, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 332, .adv_w = 96, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 339, .adv_w = 96, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 346, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 350, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 357, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 362, .adv_w = 96, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 369, .adv_w = 96, .box_w = 4, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 375, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 380, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 385, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 389, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 394, .adv_w = 96, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 400, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 405, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 412, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 417, .adv_w = 96, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 424, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 429, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 436, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 441, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 446, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 453, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 457, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 464, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 469, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 476, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 482, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 489, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 495, .adv_w = 96, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 501, .adv_w = 96, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 506, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 513, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 518, .adv_w = 96, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 525, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 530, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 537, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 542, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 549, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 554, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 561, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 566, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 573, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 577, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 584, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 589, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 596, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 601, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 608, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 613, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 618, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 622, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 629, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 633, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 640, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 644, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 651, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 656, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 663, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 668, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 675, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 680, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 687, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 692, .adv_w = 96, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 700, .adv_w = 96, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 707, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 714, .adv_w = 96, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 721, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 728, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 735, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 740, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 747, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 752, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 759, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 764, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 768, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 773, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 780, .adv_w = 96, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 788, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 795, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 800, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 805, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 810, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 817, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 821, .adv_w = 96, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 827, .adv_w = 96, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 833, .adv_w = 96, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 839, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 846, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 853, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 860, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 867, .adv_w = 96, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 873, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 878, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 883, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 888, .adv_w = 96, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 895, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 899, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 904, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 909, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 914, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 921, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 926, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 931, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 936, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 941, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 946, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 951, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 956, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 961, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 966, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 971, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 976, .adv_w = 96, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 982, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 988, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 993, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 998, .adv_w = 96, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1005, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1010, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1015, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1020, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1025, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1030, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1035, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1039, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1044, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1047, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1050, .adv_w = 96, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1055, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1059, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1063, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1066, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1069, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1074, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1078, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1082, .adv_w = 96, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1087, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1090, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1094, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1097, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1102, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1105, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1109, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1114, .adv_w = 96, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1121, .adv_w = 96, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1126, .adv_w = 96, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1131, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1134, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1138, .adv_w = 96, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1144, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1148, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1152, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1155, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1158, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1162, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1165, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1171, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1176, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1183, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1188, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1191, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1194, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1199, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1204, .adv_w = 96, .box_w = 3, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1208, .adv_w = 96, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1213, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1217, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1222, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1228, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1233, .adv_w = 96, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1240, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1244, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1249, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1254, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1259, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1263, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1270, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1274, .adv_w = 96, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1280, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1284, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1291, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1295, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1301, .adv_w = 96, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1306, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1311, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1315, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1321, .adv_w = 96, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1326, .adv_w = 96, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1333, .adv_w = 96, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1338, .adv_w = 96, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1344, .adv_w = 96, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1349, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1356, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1360, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1366, .adv_w = 96, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1371, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1376, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1381, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1386, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1391, .adv_w = 96, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1398, .adv_w = 96, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1403, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1408, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1412, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1417, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1424, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1430, .adv_w = 96, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1438, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1442, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1449, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1453, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1459, .adv_w = 96, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1463, .adv_w = 96, .box_w = 1, .box_h = 8, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 1464, .adv_w = 96, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1472, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1478, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1485, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1490, .adv_w = 96, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1496, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1500, .adv_w = 96, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1505, .adv_w = 96, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1511, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1516, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1520, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1527, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1532, .adv_w = 96, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1540, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1545, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1552, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1557, .adv_w = 96, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1563, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1568, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1575, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1579, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1586, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1590, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1597, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1602, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1607, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1611, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1618, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1623, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1630, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1635, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1642, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1649, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1656, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1663, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1670, .adv_w = 96, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1677, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1684, .adv_w = 96, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1688, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1693, .adv_w = 96, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1697, .adv_w = 96, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1704, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_2[] = {
    0x0, 0x1, 0x10, 0x11, 0x2e, 0x2f, 0x30, 0x31,
    0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
    0x40, 0x41, 0x42, 0x43, 0x48, 0x49, 0x4a, 0x4b,
    0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x58, 0x59,
    0x5e, 0x5f, 0x60, 0x61, 0x62, 0x65, 0x66, 0x69,
    0x6a
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 256, .range_length = 128, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 1024, .range_length = 96, .glyph_id_start = 129,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 1122, .range_length = 107, .glyph_id_start = 225,
        .unicode_list = unicode_list_2, .glyph_id_ofs_list = NULL, .list_length = 41, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    },
    {
        .range_start = 1231, .range_length = 43, .glyph_id_start = 266,
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
    .cmap_num = 4,
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
    .line_height = 13,          /*The maximum line height required by the font*/
    .base_line = 3,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = 0,
    .underline_thickness = 0,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if PDA_FONT_FALLBACK_10*/

