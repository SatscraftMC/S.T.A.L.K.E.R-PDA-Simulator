/* Pack a raw RGB888 file into LVGL RGB565 (little-endian uint16).
   usage: rgb565_pack <in.raw> <out.rgb565>
   Streams in chunks, so image size is not limited by RAM. */
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char ** argv)
{
    if(argc < 3) {
        fprintf(stderr, "usage: %s <in.raw> <out.rgb565>\n", argv[0]);
        return 1;
    }
    FILE * in = fopen(argv[1], "rb");
    if(!in) { perror("open in"); return 1; }
    FILE * out = fopen(argv[2], "wb");
    if(!out) { perror("open out"); fclose(in); return 1; }

    size_t chunk = ((size_t)1 << 20) - 1;   /* keep it a multiple of 3 */
    unsigned char * buf = malloc(chunk);
    unsigned char * out_buf = malloc(chunk / 3 * 2);
    if(!buf || !out_buf) { fprintf(stderr, "oom\n"); return 1; }

    size_t got;
    long total = 0;
    while((got = fread(buf, 1, chunk, in)) > 0) {
        size_t npx = got / 3;
        for(size_t i = 0; i < npx; i++) {
            unsigned r = buf[i * 3] >> 3;
            unsigned g = buf[i * 3 + 1] >> 2;
            unsigned b = buf[i * 3 + 2] >> 3;
            unsigned v = (r << 11) | (g << 5) | b;
            out_buf[i * 2] = v & 0xFF;
            out_buf[i * 2 + 1] = v >> 8;
        }
        if(fwrite(out_buf, 1, npx * 2, out) != npx * 2) {
            fprintf(stderr, "write failed\n");
            return 1;
        }
        total += npx;
    }
    fclose(in);
    fclose(out);
    fprintf(stderr, "packed %ld px -> %s\n", total, argv[2]);
    return 0;
}
