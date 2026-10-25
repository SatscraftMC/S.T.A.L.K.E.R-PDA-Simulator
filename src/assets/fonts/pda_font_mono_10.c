/*******************************************************************************
 * Size: 10 px
 * Bpp: 1
 * Opts: --font /home/aj/vibing/projects/pda-project/app/build/fonts-src/ShareTechMono-Regular.ttf --size 10 --bpp 1 --format lvgl --lv-include lvgl.h --no-compress -r 0x20-0x17F --lv-font-name pda_font_mono_10 -o /home/aj/vibing/projects/pda-project/app/src/assets/fonts/pda_font_mono_10.c
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
extern const lv_font_t pda_font_fallback_10;
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
    0xe0,

    /* U+00A0 " " */
    0x0,

    /* U+00A1 "¡" */
    0xbe,

    /* U+00A2 "¢" */
    0x5e, 0x49, 0xd0,

    /* U+00A3 "£" */
    0x74, 0x4e, 0x44, 0xf0,

    /* U+00A4 "¤" */
    0x79, 0x24, 0x92, 0x78,

    /* U+00A5 "¥" */
    0x52, 0x88, 0xe2, 0x38, 0x80,

    /* U+00A6 "¦" */
    0xe3, 0x80,

    /* U+00A7 "§" */
    0xf3, 0xde, 0x78,

    /* U+00A8 "¨" */
    0xa0,

    /* U+00A9 "©" */
    0xff, 0x73, 0x9c, 0xf7, 0xe0,

    /* U+00AA "ª" */
    0xee, 0xf0, 0x38,

    /* U+00AB "«" */
    0x76, 0xb0,

    /* U+00AC "¬" */
    0xf1, 0x10,

    /* U+00AD "­" */
    0xe0,

    /* U+00AE "®" */
    0xff, 0xf7, 0xfd, 0xc7, 0xe0,

    /* U+00AF "¯" */
    0xe0,

    /* U+00B0 "°" */
    0xf7, 0x80,

    /* U+00B1 "±" */
    0x27, 0xc8, 0xf, 0x80,

    /* U+00B2 "²" */
    0xd6, 0xc0,

    /* U+00B3 "³" */
    0xd9, 0xc0,

    /* U+00B4 "´" */
    0x60,

    /* U+00B5 "µ" */
    0xb6, 0xdf, 0x20,

    /* U+00B6 "¶" */
    0xf9, 0xf3, 0x33, 0x30,

    /* U+00B7 "·" */
    0x80,

    /* U+00B8 "¸" */
    0xa0,

    /* U+00B9 "¹" */
    0xc9, 0x2e,

    /* U+00BA "º" */
    0xf6, 0xf0, 0x38,

    /* U+00BB "»" */
    0xb6, 0xd0,

    /* U+00BC "¼" */
    0x6, 0x94, 0xa7, 0x34, 0xa5, 0x20,

    /* U+00BD "½" */
    0xb, 0x25, 0x17, 0x55, 0x92, 0x8b, 0x20,

    /* U+00BE "¾" */
    0xe, 0x6a, 0x6d, 0x55, 0x40,

    /* U+00BF "¿" */
    0x41, 0x29, 0x38,

    /* U+00C0 "À" */
    0x60, 0x88, 0xa5, 0x29, 0xc9, 0x88,

    /* U+00C1 "Á" */
    0x11, 0x8, 0xa5, 0x29, 0xc9, 0x88,

    /* U+00C2 "Â" */
    0x22, 0x88, 0xa5, 0x29, 0xc9, 0x88,

    /* U+00C3 "Ã" */
    0x22, 0x88, 0xa5, 0x29, 0xc9, 0x88,

    /* U+00C4 "Ä" */
    0x51, 0x14, 0xa5, 0x39, 0x31,

    /* U+00C5 "Å" */
    0x32, 0x8c, 0xa5, 0x25, 0xf1, 0x88,

    /* U+00C6 "Æ" */
    0x3b, 0x18, 0xb7, 0x4a, 0x60,

    /* U+00C7 "Ç" */
    0xf2, 0x49, 0x39, 0x60,

    /* U+00C8 "È" */
    0xc7, 0xc9, 0xe4, 0xe0,

    /* U+00C9 "É" */
    0x2b, 0xc9, 0xe4, 0xe0,

    /* U+00CA "Ê" */
    0x57, 0xc9, 0xe4, 0xe0,

    /* U+00CB "Ë" */
    0xbe, 0x4f, 0x27,

    /* U+00CC "Ì" */
    0x8b, 0xa4, 0x92, 0xe0,

    /* U+00CD "Í" */
    0x3b, 0xa4, 0x92, 0xe0,

    /* U+00CE "Î" */
    0x57, 0xa4, 0x92, 0xe0,

    /* U+00CF "Ï" */
    0xbd, 0x24, 0x97,

    /* U+00D0 "Ð" */
    0x7a, 0x53, 0xd4, 0xa5, 0xe0,

    /* U+00D1 "Ñ" */
    0x42, 0xdd, 0xdd, 0xbb, 0xb0,

    /* U+00D2 "Ò" */
    0x42, 0xf9, 0x99, 0x99, 0xf0,

    /* U+00D3 "Ó" */
    0x24, 0xf9, 0x99, 0x99, 0xf0,

    /* U+00D4 "Ô" */
    0xf, 0xf9, 0x99, 0x99, 0xf0,

    /* U+00D5 "Õ" */
    0x42, 0xf9, 0x99, 0x99, 0xf0,

    /* U+00D6 "Ö" */
    0x6f, 0x99, 0x99, 0x9f,

    /* U+00D7 "×" */
    0x8d, 0xc0,

    /* U+00D8 "Ø" */
    0x2e, 0xbb, 0xdd, 0xd7, 0x80,

    /* U+00D9 "Ù" */
    0xc2, 0x99, 0x99, 0x99, 0xf0,

    /* U+00DA "Ú" */
    0x34, 0x99, 0x99, 0x99, 0xf0,

    /* U+00DB "Û" */
    0xf, 0x99, 0x99, 0x99, 0xf0,

    /* U+00DC "Ü" */
    0xa9, 0x99, 0x99, 0x9f,

    /* U+00DD "Ý" */
    0x11, 0x22, 0xa5, 0x10, 0x84, 0x20,

    /* U+00DE "Þ" */
    0x8f, 0x99, 0x9f, 0x80,

    /* U+00DF "ß" */
    0xf9, 0xba, 0xb9, 0xb0,

    /* U+00E0 "à" */
    0x41, 0x3c, 0x2f, 0x4b, 0xe0,

    /* U+00E1 "á" */
    0x20, 0x3c, 0x2f, 0x4b, 0xe0,

    /* U+00E2 "â" */
    0x60, 0x3c, 0x2f, 0x4b, 0xe0,

    /* U+00E3 "ã" */
    0xf0, 0x3c, 0x2f, 0x4b, 0xe0,

    /* U+00E4 "ä" */
    0x90, 0x3c, 0x2f, 0x4b, 0xe0,

    /* U+00E5 "å" */
    0x65, 0x19, 0xe1, 0x7a, 0x5f,

    /* U+00E6 "æ" */
    0xf9, 0x7f, 0x4d, 0x80,

    /* U+00E7 "ç" */
    0xf2, 0x4e, 0xd8,

    /* U+00E8 "è" */
    0x42, 0xf9, 0xf8, 0xf0,

    /* U+00E9 "é" */
    0x20, 0xf9, 0xf8, 0xf0,

    /* U+00EA "ê" */
    0x60, 0xf9, 0xf8, 0xf0,

    /* U+00EB "ë" */
    0x90, 0xf9, 0xf8, 0xf0,

    /* U+00EC "ì" */
    0x43, 0x24, 0x98,

    /* U+00ED "í" */
    0x63, 0x24, 0x98,

    /* U+00EE "î" */
    0x57, 0x24, 0x98,

    /* U+00EF "ï" */
    0xa3, 0x24, 0x98,

    /* U+00F0 "ð" */
    0x26, 0x5f, 0x99, 0x9f,

    /* U+00F1 "ñ" */
    0x70, 0xf9, 0x99, 0x90,

    /* U+00F2 "ò" */
    0x40, 0xf9, 0x99, 0xf0,

    /* U+00F3 "ó" */
    0x20, 0xf9, 0x99, 0xf0,

    /* U+00F4 "ô" */
    0x60, 0xf9, 0x99, 0xf0,

    /* U+00F5 "õ" */
    0x60, 0xf9, 0x99, 0xf0,

    /* U+00F6 "ö" */
    0x90, 0xf9, 0x99, 0xf0,

    /* U+00F7 "÷" */
    0x43, 0x84,

    /* U+00F8 "ø" */
    0x2f, 0xbd, 0xdf, 0x40,

    /* U+00F9 "ù" */
    0x40, 0x99, 0x99, 0xf0,

    /* U+00FA "ú" */
    0x20, 0x99, 0x99, 0xf0,

    /* U+00FB "û" */
    0x60, 0x99, 0x99, 0xf0,

    /* U+00FC "ü" */
    0x90, 0x99, 0x99, 0xf0,

    /* U+00FD "ý" */
    0x24, 0x9a, 0xa6, 0x64, 0xc0,

    /* U+00FE "þ" */
    0x88, 0xf9, 0x99, 0xf8, 0x80,

    /* U+00FF "ÿ" */
    0xa0, 0x9a, 0xa6, 0x64, 0xc0,

    /* U+0131 "ı" */
    0xc9, 0x26,

    /* U+0141 "Ł" */
    0x44, 0x6c, 0x44, 0x70,

    /* U+0142 "ł" */
    0xc4, 0x4e, 0x44, 0x70,

    /* U+0152 "Œ" */
    0xfe, 0x49, 0x26, 0x92, 0x4f, 0xc0,

    /* U+0153 "œ" */
    0xfd, 0x6f, 0x4d, 0x80,

    /* U+0160 "Š" */
    0xab, 0xc9, 0xc9, 0xe0,

    /* U+0161 "š" */
    0xab, 0xce, 0x78,

    /* U+0178 "Ÿ" */
    0x54, 0x54, 0xa2, 0x10, 0x84,

    /* U+017D "Ž" */
    0xa4, 0xf2, 0x24, 0x48, 0xe0,

    /* U+017E "ž" */
    0x60, 0xf2, 0x48, 0xe0
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
    {.bitmap_index = 313, .adv_w = 86, .box_w = 3, .box_h = 1, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 314, .adv_w = 86, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 315, .adv_w = 86, .box_w = 1, .box_h = 7, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 316, .adv_w = 86, .box_w = 3, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 319, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 323, .adv_w = 86, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 327, .adv_w = 86, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 332, .adv_w = 86, .box_w = 1, .box_h = 9, .ofs_x = 2, .ofs_y = -1},
    {.bitmap_index = 334, .adv_w = 86, .box_w = 3, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 337, .adv_w = 86, .box_w = 3, .box_h = 1, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 338, .adv_w = 86, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 343, .adv_w = 86, .box_w = 3, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 346, .adv_w = 86, .box_w = 3, .box_h = 4, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 348, .adv_w = 86, .box_w = 4, .box_h = 3, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 350, .adv_w = 86, .box_w = 3, .box_h = 1, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 351, .adv_w = 86, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 356, .adv_w = 86, .box_w = 3, .box_h = 1, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 357, .adv_w = 86, .box_w = 3, .box_h = 3, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 359, .adv_w = 86, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 363, .adv_w = 86, .box_w = 2, .box_h = 5, .ofs_x = 2, .ofs_y = 2},
    {.bitmap_index = 365, .adv_w = 86, .box_w = 2, .box_h = 5, .ofs_x = 2, .ofs_y = 2},
    {.bitmap_index = 367, .adv_w = 86, .box_w = 2, .box_h = 2, .ofs_x = 2, .ofs_y = 6},
    {.bitmap_index = 368, .adv_w = 86, .box_w = 3, .box_h = 7, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 371, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 375, .adv_w = 86, .box_w = 1, .box_h = 1, .ofs_x = 2, .ofs_y = 3},
    {.bitmap_index = 376, .adv_w = 86, .box_w = 2, .box_h = 2, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 377, .adv_w = 86, .box_w = 3, .box_h = 5, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 379, .adv_w = 86, .box_w = 3, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 382, .adv_w = 86, .box_w = 3, .box_h = 4, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 384, .adv_w = 86, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 390, .adv_w = 86, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 397, .adv_w = 86, .box_w = 4, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 402, .adv_w = 86, .box_w = 3, .box_h = 7, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 405, .adv_w = 86, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 411, .adv_w = 86, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 417, .adv_w = 86, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 423, .adv_w = 86, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 429, .adv_w = 86, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 434, .adv_w = 86, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 440, .adv_w = 86, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 445, .adv_w = 86, .box_w = 3, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 449, .adv_w = 86, .box_w = 3, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 453, .adv_w = 86, .box_w = 3, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 457, .adv_w = 86, .box_w = 3, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 461, .adv_w = 86, .box_w = 3, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 464, .adv_w = 86, .box_w = 3, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 468, .adv_w = 86, .box_w = 3, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 472, .adv_w = 86, .box_w = 3, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 476, .adv_w = 86, .box_w = 3, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 479, .adv_w = 86, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 484, .adv_w = 86, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 489, .adv_w = 86, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 494, .adv_w = 86, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 499, .adv_w = 86, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 504, .adv_w = 86, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 509, .adv_w = 86, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 513, .adv_w = 86, .box_w = 3, .box_h = 4, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 515, .adv_w = 86, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 520, .adv_w = 86, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 525, .adv_w = 86, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 530, .adv_w = 86, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 535, .adv_w = 86, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 539, .adv_w = 86, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 545, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 549, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 553, .adv_w = 86, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 558, .adv_w = 86, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 563, .adv_w = 86, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 568, .adv_w = 86, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 573, .adv_w = 86, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 578, .adv_w = 86, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 583, .adv_w = 86, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 587, .adv_w = 86, .box_w = 3, .box_h = 7, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 590, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 594, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 598, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 602, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 606, .adv_w = 86, .box_w = 3, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 609, .adv_w = 86, .box_w = 3, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 612, .adv_w = 86, .box_w = 3, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 615, .adv_w = 86, .box_w = 3, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 618, .adv_w = 86, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 622, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 626, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 630, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 634, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 638, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 642, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 646, .adv_w = 86, .box_w = 3, .box_h = 5, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 648, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 652, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 656, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 660, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 664, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 668, .adv_w = 86, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 673, .adv_w = 86, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 678, .adv_w = 86, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 683, .adv_w = 86, .box_w = 3, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 685, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 689, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 693, .adv_w = 86, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 699, .adv_w = 86, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 703, .adv_w = 86, .box_w = 3, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 707, .adv_w = 86, .box_w = 3, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 710, .adv_w = 86, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 715, .adv_w = 86, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 720, .adv_w = 86, .box_w = 4, .box_h = 7, .ofs_x = 2, .ofs_y = 0}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_2[] = {
    0x0, 0x10, 0x11, 0x21, 0x22, 0x2f, 0x30, 0x47,
    0x4c, 0x4d
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 95, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 160, .range_length = 96, .glyph_id_start = 96,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 305, .range_length = 78, .glyph_id_start = 192,
        .unicode_list = unicode_list_2, .glyph_id_ofs_list = NULL, .list_length = 10, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
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
    .cmap_num = 3,
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
    .fallback = &pda_font_fallback_10,
#endif
    .user_data = NULL,
};



#endif /*#if PDA_FONT_MONO_10*/

