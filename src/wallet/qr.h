#ifndef PDA_WALLET_QR_H
#define PDA_WALLET_QR_H

#include <stdbool.h>
#include <stdint.h>

/* One QR symbol, rasterised and ready to be handed to an LVGL canvas.
 *
 * The pixels are ARGB8888: 0xFF000000 for a module that is set and
 * 0xFFFFFFFF for the background and the quiet zone, at QR_MIN_SCALE pixels
 * a module or more - the plain black-on-white a camera wants, with no
 * scaling or anti-aliasing left for the display to do. A long bolt11 comes
 * out around version 11, which is 61 modules, and 4 pixels a module keeps
 * it readable on a 5" screen and straight through a screenshot.
 *
 * Encoding is qrcodegen (Project Nayuki, MIT), vendored next to this file. */

#define QR_QUIET     4       /* modules of quiet zone on every side */
#define QR_MIN_SCALE 3       /* px a module may not go below */
#define QR_MAX_SCALE 8       /* ...and may not go above either */
#define QR_MAX_SIDE  288     /* px: the largest code the REQUEST screen holds */

typedef struct {
    uint32_t * px;           /* side*side ARGB8888; NULL when it failed */
    int side;                /* px; = (modules + 2*QR_QUIET) * scale */
    int modules;             /* modules across, without the quiet zone */
    int scale;               /* px a module */
    int version;             /* QR version that was used */
    const char * err;        /* why it failed - NULL when it did not */
} qr_image_t;

/* Encode text at ECC level L. Fails (with out->err set) when the text does
 * not fit the versions the screen can draw, or when out is NULL. */
bool qr_render(const char * text, qr_image_t * out);

/* let go of an image that came out of qr_render() */
void qr_release(qr_image_t * img);

#endif
