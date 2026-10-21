/*******************************************************************************
 * Size: 14 px
 * Bpp: 1
 * Opts: --font /home/aj/pda-project/app/build/fonts-src/ShareTechMono-Regular.ttf --size 14 --bpp 1 --format lvgl --lv-include lvgl.h --no-compress -r 0x20-0x17F --lv-font-name pda_font_mono_14 -o /home/aj/pda-project/app/src/assets/fonts/pda_font_mono_14.c
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
extern const lv_font_t pda_font_fallback_14;
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
    0x6c, 0xc0,

    /* U+00A0 " " */
    0x0,

    /* U+00A1 "¡" */
    0xbf, 0x80,

    /* U+00A2 "¢" */
    0x22, 0x78, 0x88, 0x88, 0x72, 0x20,

    /* U+00A3 "£" */
    0x3a, 0x10, 0x8f, 0x21, 0x8, 0x47, 0xc0,

    /* U+00A4 "¤" */
    0x0, 0xf9, 0x12, 0x24, 0x48, 0x9f, 0x0,

    /* U+00A5 "¥" */
    0x44, 0xc8, 0xa1, 0x41, 0xf, 0x84, 0x3e, 0x10,
    0x20,

    /* U+00A6 "¦" */
    0xf8, 0xf8,

    /* U+00A7 "§" */
    0xf8, 0x8f, 0x99, 0xf1, 0x1e,

    /* U+00A8 "¨" */
    0x90,

    /* U+00A9 "©" */
    0x7d, 0x6, 0xed, 0x1a, 0x34, 0x68, 0xdd, 0x82,
    0xf8,

    /* U+00AA "ª" */
    0x71, 0xf9, 0xf0, 0x0, 0xf,

    /* U+00AB "«" */
    0x5b, 0xaa, 0xb5,

    /* U+00AC "¬" */
    0xfc, 0x10, 0x41,

    /* U+00AD "­" */
    0xf8,

    /* U+00AE "®" */
    0x7d, 0x6, 0xed, 0x5a, 0xb7, 0x6a, 0xd5, 0x82,
    0xf8,

    /* U+00AF "¯" */
    0xf0,

    /* U+00B0 "°" */
    0xf9, 0x9f,

    /* U+00B1 "±" */
    0x21, 0x3e, 0x42, 0x3, 0xe0,

    /* U+00B2 "²" */
    0x71, 0x13, 0x24, 0xf0,

    /* U+00B3 "³" */
    0xe4, 0xe2, 0x78,

    /* U+00B4 "´" */
    0x68,

    /* U+00B5 "µ" */
    0x8c, 0x63, 0x18, 0xc7, 0xf0, 0x80,

    /* U+00B6 "¶" */
    0x7c, 0x63, 0x17, 0x8c, 0x63, 0x18, 0xc0,

    /* U+00B7 "·" */
    0x80,

    /* U+00B8 "¸" */
    0xb7,

    /* U+00B9 "¹" */
    0x6a, 0x22, 0x22, 0x70,

    /* U+00BA "º" */
    0xf9, 0x99, 0xf0, 0x0, 0xf,

    /* U+00BB "»" */
    0xaa, 0xdd, 0xaa,

    /* U+00BC "¼" */
    0x4, 0xb, 0x12, 0x44, 0x89, 0x93, 0x2a, 0xf6,
    0x2c, 0xc9, 0x12, 0x4, 0x0,

    /* U+00BD "½" */
    0x4, 0xb, 0x22, 0x44, 0x89, 0xd4, 0xa9, 0xd2,
    0x28, 0x91, 0x32, 0x4, 0x0,

    /* U+00BE "¾" */
    0x4, 0xb, 0x91, 0x42, 0x89, 0x8b, 0x1a, 0xf6,
    0x2c, 0x49, 0x12, 0x4, 0x0,

    /* U+00BF "¿" */
    0x20, 0x22, 0x68, 0x88, 0x70,

    /* U+00C0 "À" */
    0x60, 0xc3, 0xc, 0x51, 0x44, 0x92, 0xfa, 0x28,
    0xe1,

    /* U+00C1 "Á" */
    0x10, 0x83, 0xc, 0x51, 0x44, 0x92, 0xfa, 0x28,
    0xe1,

    /* U+00C2 "Â" */
    0x31, 0x63, 0xc, 0x51, 0x44, 0x92, 0xfa, 0x28,
    0xe1,

    /* U+00C3 "Ã" */
    0x60, 0x63, 0xc, 0x31, 0x24, 0x92, 0x7a, 0x38,
    0x61,

    /* U+00C4 "Ä" */
    0x48, 0xc3, 0x1e, 0x49, 0x24, 0x9e, 0x86, 0x18,
    0x40,

    /* U+00C5 "Å" */
    0x38, 0x50, 0xe1, 0x42, 0x85, 0x1b, 0x22, 0x7c,
    0x8b, 0x1c, 0x10,

    /* U+00C6 "Æ" */
    0x1e, 0x28, 0x28, 0x28, 0x2e, 0x48, 0x78, 0x48,
    0x48, 0xcf,

    /* U+00C7 "Ç" */
    0x7c, 0x21, 0x8, 0x42, 0x10, 0x83, 0xcc, 0x23,
    0x0,

    /* U+00C8 "È" */
    0x42, 0xf8, 0x88, 0xf8, 0x88, 0x8f,

    /* U+00C9 "É" */
    0x26, 0xf8, 0x88, 0xf8, 0x88, 0x8f,

    /* U+00CA "Ê" */
    0x6d, 0xf8, 0x88, 0xf8, 0x88, 0x8f,

    /* U+00CB "Ë" */
    0x9f, 0x88, 0x8f, 0x88, 0x88, 0xf0,

    /* U+00CC "Ì" */
    0xc4, 0xf4, 0x44, 0x44, 0x44, 0x4f,

    /* U+00CD "Í" */
    0x64, 0xf4, 0x44, 0x44, 0x44, 0x4f,

    /* U+00CE "Î" */
    0x6b, 0xf4, 0x44, 0x44, 0x44, 0x4f,

    /* U+00CF "Ï" */
    0xbd, 0x24, 0x92, 0x4b, 0x80,

    /* U+00D0 "Ð" */
    0x79, 0x14, 0x51, 0xf5, 0x14, 0x51, 0x45, 0xe0,

    /* U+00D1 "Ñ" */
    0x68, 0xf3, 0x9c, 0xd6, 0xb5, 0xac, 0xe7, 0x30,

    /* U+00D2 "Ò" */
    0x61, 0x1d, 0x18, 0xc6, 0x31, 0x8c, 0x62, 0xe0,

    /* U+00D3 "Ó" */
    0x31, 0x1d, 0x18, 0xc6, 0x31, 0x8c, 0x62, 0xe0,

    /* U+00D4 "Ô" */
    0x22, 0x9d, 0x18, 0xc6, 0x31, 0x8c, 0x62, 0xe0,

    /* U+00D5 "Õ" */
    0x68, 0xdd, 0x18, 0xc6, 0x31, 0x8c, 0x62, 0xe0,

    /* U+00D6 "Ö" */
    0x4b, 0xa3, 0x18, 0xc6, 0x31, 0x8c, 0x5c,

    /* U+00D7 "×" */
    0xa, 0xcc, 0xe4, 0x80,

    /* U+00D8 "Ø" */
    0x13, 0xa7, 0x3a, 0xd6, 0xb5, 0xce, 0x5c, 0x88,
    0x0,

    /* U+00D9 "Ù" */
    0x41, 0x23, 0x18, 0xc6, 0x31, 0x8c, 0x62, 0xe0,

    /* U+00DA "Ú" */
    0x11, 0x23, 0x18, 0xc6, 0x31, 0x8c, 0x62, 0xe0,

    /* U+00DB "Û" */
    0x32, 0xa3, 0x18, 0xc6, 0x31, 0x8c, 0x62, 0xe0,

    /* U+00DC "Ü" */
    0x4c, 0x63, 0x18, 0xc6, 0x31, 0x8c, 0x5c,

    /* U+00DD "Ý" */
    0x18, 0x21, 0x12, 0x22, 0x85, 0x4, 0x8, 0x10,
    0x20, 0x40, 0x80,

    /* U+00DE "Þ" */
    0x84, 0x3d, 0x18, 0xc6, 0x3e, 0x84, 0x0,

    /* U+00DF "ß" */
    0x74, 0x63, 0x7a, 0x52, 0xf1, 0x8d, 0xc0,

    /* U+00E0 "à" */
    0x60, 0xc0, 0x3c, 0x8, 0x27, 0xa2, 0x8b, 0xf0,

    /* U+00E1 "á" */
    0x10, 0xc0, 0x3c, 0x8, 0x27, 0xa2, 0x8b, 0xf0,

    /* U+00E2 "â" */
    0x71, 0x60, 0x3c, 0x8, 0x27, 0xa2, 0x8b, 0xf0,

    /* U+00E3 "ã" */
    0xe2, 0x40, 0x3c, 0x8, 0x27, 0xa2, 0x8b, 0xf0,

    /* U+00E4 "ä" */
    0x50, 0x7, 0x2, 0x9, 0xe8, 0xa2, 0xfc,

    /* U+00E5 "å" */
    0x71, 0x45, 0x1c, 0xf0, 0x20, 0x9e, 0x8a, 0x2f,
    0xc0,

    /* U+00E6 "æ" */
    0xfc, 0x24, 0x4b, 0xf9, 0x12, 0x3b, 0x80,

    /* U+00E7 "ç" */
    0x78, 0x88, 0x88, 0x76, 0x17,

    /* U+00E8 "è" */
    0x61, 0x80, 0xe8, 0xc7, 0xf0, 0x83, 0xc0,

    /* U+00E9 "é" */
    0x11, 0x0, 0xe8, 0xc7, 0xf0, 0x83, 0xc0,

    /* U+00EA "ê" */
    0x72, 0xc0, 0xe8, 0xc7, 0xf0, 0x83, 0xc0,

    /* U+00EB "ë" */
    0x50, 0x1d, 0x18, 0xfe, 0x10, 0x70,

    /* U+00EC "ì" */
    0x61, 0x1, 0xc2, 0x10, 0x84, 0x21, 0xc0,

    /* U+00ED "í" */
    0x11, 0x1, 0xc2, 0x10, 0x84, 0x21, 0xc0,

    /* U+00EE "î" */
    0x22, 0x81, 0xc2, 0x10, 0x84, 0x21, 0xc0,

    /* U+00EF "ï" */
    0x50, 0x38, 0x42, 0x10, 0x84, 0x38,

    /* U+00F0 "ð" */
    0x13, 0x9a, 0x10, 0xbe, 0x31, 0x8c, 0x5c,

    /* U+00F1 "ñ" */
    0x68, 0xc1, 0xe8, 0xc6, 0x31, 0x8c, 0x40,

    /* U+00F2 "ò" */
    0x61, 0x80, 0xe8, 0xc6, 0x31, 0x8b, 0x80,

    /* U+00F3 "ó" */
    0x33, 0x0, 0xe8, 0xc6, 0x31, 0x8b, 0x80,

    /* U+00F4 "ô" */
    0x72, 0x80, 0xe8, 0xc6, 0x31, 0x8b, 0x80,

    /* U+00F5 "õ" */
    0x68, 0xc0, 0xe8, 0xc6, 0x31, 0x8b, 0x80,

    /* U+00F6 "ö" */
    0x48, 0x1d, 0x18, 0xc6, 0x31, 0x70,

    /* U+00F7 "÷" */
    0x20, 0x1, 0xf0, 0x0, 0x80,

    /* U+00F8 "ø" */
    0x10, 0x9d, 0x3a, 0xd6, 0xb9, 0xf2, 0x0,

    /* U+00F9 "ù" */
    0x41, 0x1, 0x18, 0xc6, 0x31, 0x8b, 0xc0,

    /* U+00FA "ú" */
    0x11, 0x1, 0x18, 0xc6, 0x31, 0x8b, 0xc0,

    /* U+00FB "û" */
    0x72, 0x81, 0x18, 0xc6, 0x31, 0x8b, 0xc0,

    /* U+00FC "ü" */
    0x50, 0x23, 0x18, 0xc6, 0x31, 0x78,

    /* U+00FD "ý" */
    0x10, 0x80, 0x23, 0xc9, 0x24, 0x9c, 0x30, 0xc3,
    0x18,

    /* U+00FE "þ" */
    0x84, 0x21, 0xe8, 0xc6, 0x31, 0x8f, 0xa1, 0x0,

    /* U+00FF "ÿ" */
    0x48, 0x0, 0x21, 0x49, 0x24, 0x9a, 0x30, 0xc3,
    0x18,

    /* U+0131 "ı" */
    0xe1, 0x8, 0x42, 0x10, 0xe0,

    /* U+0141 "Ł" */
    0x41, 0x4, 0x1c, 0xc1, 0x4, 0x10, 0x41, 0xf0,

    /* U+0142 "ł" */
    0xe1, 0x8, 0x47, 0x30, 0x84, 0x21, 0xc0,

    /* U+0152 "Œ" */
    0x7f, 0x12, 0x24, 0x48, 0xf1, 0x22, 0x44, 0x88,
    0xfc,

    /* U+0153 "œ" */
    0xff, 0x26, 0x4c, 0xf9, 0x12, 0x3b, 0x80,

    /* U+0160 "Š" */
    0x59, 0x9f, 0x8, 0x41, 0xc1, 0x8, 0x43, 0xe0,

    /* U+0161 "š" */
    0xd6, 0xf, 0x88, 0xf1, 0x1f,

    /* U+0178 "Ÿ" */
    0x4a, 0x1c, 0x96, 0x50, 0xc2, 0x8, 0x20, 0x82,
    0x0,

    /* U+017D "Ž" */
    0x51, 0xbe, 0x31, 0x18, 0x84, 0x42, 0x21, 0xf0,

    /* U+017E "ž" */
    0xf6, 0xf, 0x32, 0x64, 0x8f
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
    {.bitmap_index = 544, .adv_w = 121, .box_w = 5, .box_h = 2, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 546, .adv_w = 121, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 547, .adv_w = 121, .box_w = 1, .box_h = 9, .ofs_x = 3, .ofs_y = -2},
    {.bitmap_index = 549, .adv_w = 121, .box_w = 4, .box_h = 11, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 555, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 562, .adv_w = 121, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 569, .adv_w = 121, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 578, .adv_w = 121, .box_w = 1, .box_h = 13, .ofs_x = 3, .ofs_y = -2},
    {.bitmap_index = 580, .adv_w = 121, .box_w = 4, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 585, .adv_w = 121, .box_w = 4, .box_h = 1, .ofs_x = 2, .ofs_y = 8},
    {.bitmap_index = 586, .adv_w = 121, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 595, .adv_w = 121, .box_w = 4, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 600, .adv_w = 121, .box_w = 4, .box_h = 6, .ofs_x = 2, .ofs_y = 1},
    {.bitmap_index = 603, .adv_w = 121, .box_w = 6, .box_h = 4, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 606, .adv_w = 121, .box_w = 5, .box_h = 1, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 607, .adv_w = 121, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 616, .adv_w = 121, .box_w = 4, .box_h = 1, .ofs_x = 2, .ofs_y = 8},
    {.bitmap_index = 617, .adv_w = 121, .box_w = 4, .box_h = 4, .ofs_x = 2, .ofs_y = 5},
    {.bitmap_index = 619, .adv_w = 121, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 624, .adv_w = 121, .box_w = 4, .box_h = 7, .ofs_x = 2, .ofs_y = 3},
    {.bitmap_index = 628, .adv_w = 121, .box_w = 3, .box_h = 7, .ofs_x = 2, .ofs_y = 3},
    {.bitmap_index = 631, .adv_w = 121, .box_w = 3, .box_h = 2, .ofs_x = 2, .ofs_y = 8},
    {.bitmap_index = 632, .adv_w = 121, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 638, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 645, .adv_w = 121, .box_w = 1, .box_h = 1, .ofs_x = 3, .ofs_y = 4},
    {.bitmap_index = 646, .adv_w = 121, .box_w = 2, .box_h = 4, .ofs_x = 3, .ofs_y = -3},
    {.bitmap_index = 647, .adv_w = 121, .box_w = 4, .box_h = 7, .ofs_x = 2, .ofs_y = 3},
    {.bitmap_index = 651, .adv_w = 121, .box_w = 4, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 656, .adv_w = 121, .box_w = 4, .box_h = 6, .ofs_x = 2, .ofs_y = 1},
    {.bitmap_index = 659, .adv_w = 121, .box_w = 7, .box_h = 14, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 672, .adv_w = 121, .box_w = 7, .box_h = 14, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 685, .adv_w = 121, .box_w = 7, .box_h = 14, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 698, .adv_w = 121, .box_w = 4, .box_h = 9, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 703, .adv_w = 121, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 712, .adv_w = 121, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 721, .adv_w = 121, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 730, .adv_w = 121, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 739, .adv_w = 121, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 748, .adv_w = 121, .box_w = 7, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 759, .adv_w = 121, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 769, .adv_w = 121, .box_w = 5, .box_h = 13, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 778, .adv_w = 121, .box_w = 4, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 784, .adv_w = 121, .box_w = 4, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 790, .adv_w = 121, .box_w = 4, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 796, .adv_w = 121, .box_w = 4, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 802, .adv_w = 121, .box_w = 4, .box_h = 12, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 808, .adv_w = 121, .box_w = 4, .box_h = 12, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 814, .adv_w = 121, .box_w = 4, .box_h = 12, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 820, .adv_w = 121, .box_w = 3, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 825, .adv_w = 121, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 833, .adv_w = 121, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 841, .adv_w = 121, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 849, .adv_w = 121, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 857, .adv_w = 121, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 865, .adv_w = 121, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 873, .adv_w = 121, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 880, .adv_w = 121, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 884, .adv_w = 121, .box_w = 5, .box_h = 13, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 893, .adv_w = 121, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 901, .adv_w = 121, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 909, .adv_w = 121, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 917, .adv_w = 121, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 924, .adv_w = 121, .box_w = 7, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 935, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 942, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 949, .adv_w = 121, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 957, .adv_w = 121, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 965, .adv_w = 121, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 973, .adv_w = 121, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 981, .adv_w = 121, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 988, .adv_w = 121, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 997, .adv_w = 121, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1004, .adv_w = 121, .box_w = 4, .box_h = 10, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 1009, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1016, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1023, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1030, .adv_w = 121, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1036, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1043, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1050, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1057, .adv_w = 121, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1063, .adv_w = 121, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1070, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1077, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1084, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1091, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1098, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1105, .adv_w = 121, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1111, .adv_w = 121, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 1116, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1123, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1130, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1137, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1144, .adv_w = 121, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1150, .adv_w = 121, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1159, .adv_w = 121, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1167, .adv_w = 121, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1176, .adv_w = 121, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1181, .adv_w = 121, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1189, .adv_w = 121, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1196, .adv_w = 121, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1205, .adv_w = 121, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1212, .adv_w = 121, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1220, .adv_w = 121, .box_w = 4, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 1225, .adv_w = 121, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1234, .adv_w = 121, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1242, .adv_w = 121, .box_w = 4, .box_h = 10, .ofs_x = 2, .ofs_y = 0}
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
const lv_font_t pda_font_mono_14 = {
#else
lv_font_t pda_font_mono_14 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 15,          /*The maximum line height required by the font*/
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
    .fallback = &pda_font_fallback_14,
#endif
    .user_data = NULL,
};



#endif /*#if PDA_FONT_MONO_14*/

