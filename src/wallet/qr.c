#include "wallet/qr.h"
#include "wallet/qrcodegen.h"

#include <stdlib.h>
#include <string.h>

/* qrcodegen wants two scratch buffers the size of the biggest symbol it may
 * make. Bounded by the version the screen can draw, not by version 40, but
 * the library is happy with the worst case and it is under 8KB. */
#define QR_BUF_LEN  qrcodegen_BUFFER_LEN_FOR_VERSION(qrcodegen_VERSION_MAX)

/* The versions this screen can hold: at QR_MIN_SCALE a module, the code plus
 * its quiet zone may not be wider than QR_MAX_SIDE. */
static int max_version(void)
{
    int max_modules = QR_MAX_SIDE / QR_MIN_SCALE - 2 * QR_QUIET;
    int v = (max_modules - 17) / 4;                 /* modules = 4v + 17 */
    if(v < qrcodegen_VERSION_MIN) return qrcodegen_VERSION_MIN;
    if(v > qrcodegen_VERSION_MAX) return qrcodegen_VERSION_MAX;
    return v;
}

bool qr_render(const char * text, qr_image_t * out)
{
    if(!out) return false;
    memset(out, 0, sizeof(*out));
    if(!text || !*text) {
        out->err = "there is nothing to encode";
        return false;
    }

    /* the encode step: ECC L, the smallest symbol that fits. Both scratch
       buffers are ours alone - the UI is single threaded. */
    static uint8_t temp[QR_BUF_LEN];
    static uint8_t data[QR_BUF_LEN];
    int ver = max_version();
    if(!qrcodegen_encodeText(text, temp, data, qrcodegen_Ecc_LOW,
                             qrcodegen_VERSION_MIN, ver, qrcodegen_Mask_AUTO,
                             false)) {
        out->err = "too long for a QR code";
        return false;
    }

    int size = qrcodegen_getSize(data);
    if(size <= 0) {
        out->err = "the encoder made nothing";
        return false;
    }

    /* pixels a module: big enough to read, small enough to fit the frame */
    int total = size + 2 * QR_QUIET;
    int scale = QR_MAX_SIDE / total;
    if(scale > QR_MAX_SCALE) scale = QR_MAX_SCALE;
    if(scale < QR_MIN_SCALE) scale = QR_MIN_SCALE;
    int side = total * scale;
    if(side > QR_MAX_SIDE || side <= 0) {
        out->err = "too long for a QR code";
        return false;
    }

    uint32_t * px = malloc((size_t)side * (size_t)side * sizeof(uint32_t));
    if(!px) {
        out->err = "out of memory";
        return false;
    }

    /* white everywhere, then the dark modules over it */
    for(int i = 0; i < side * side; i++) px[i] = 0xFFFFFFFFu;
    for(int y = 0; y < size; y++) {
        for(int x = 0; x < size; x++) {
            if(!qrcodegen_getModule(data, x, y)) continue;
            int x0 = (QR_QUIET + x) * scale;
            int y0 = (QR_QUIET + y) * scale;
            for(int dy = 0; dy < scale; dy++) {
                uint32_t * row = px + (size_t)(y0 + dy) * (size_t)side + (size_t)x0;
                for(int dx = 0; dx < scale; dx++) row[dx] = 0xFF000000u;
            }
        }
    }

    out->px = px;
    out->side = side;
    out->modules = size;
    out->scale = scale;
    out->version = (size - 17) / 4;
    out->err = NULL;
    return true;
}

void qr_release(qr_image_t * img)
{
    if(!img) return;
    free(img->px);
    memset(img, 0, sizeof(*img));
}
