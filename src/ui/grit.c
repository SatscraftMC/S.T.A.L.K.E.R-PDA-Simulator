#include "ui/grit.h"
#include "ui/palette.h"

#include <stdlib.h>

/* The texture of the guide screens: a dark surface with grain, scanlines,
   a few stains and a vignette. It never changes, so it is drawn once into a
   canvas and left there. */

static uint32_t s_rand = 0x1F123BB5u;

static uint32_t rnd(void)
{
    s_rand ^= s_rand << 13;
    s_rand ^= s_rand >> 17;
    s_rand ^= s_rand << 5;
    return s_rand;
}

static uint16_t rgb565(uint32_t rgb)
{
    return (uint16_t)(((rgb >> 19 & 31) << 11) | ((rgb >> 10 & 63) << 5) |
                      (rgb >> 3 & 31));
}

/* an object that is only ever decoration: it never takes the click and it
   never scrolls */
static lv_obj_t * inert(lv_obj_t * parent)
{
    lv_obj_t * o = lv_obj_create(parent);
    lv_obj_set_style_bg_opa(o, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(o, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(o, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(o, 0, LV_PART_MAIN);
    lv_obj_set_style_outline_width(o, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(o, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(o, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scrollable(o, false);
    lv_obj_set_clickable(o, false);
    return o;
}

lv_obj_t * grit_backdrop(lv_obj_t * parent, int w, int h)
{
    if(w <= 0 || h <= 0) return NULL;

    uint16_t * buf = malloc((size_t)w * (size_t)h * sizeof(uint16_t));
    if(!buf) return NULL;

    /* a handful of stains: soft dark blotches */
    int sx[10], sy[10], sr[10], sd[10];
    for(int i = 0; i < 10; i++) {
        sx[i] = (int)(rnd() % (uint32_t)w);
        sy[i] = (int)(rnd() % (uint32_t)h);
        sr[i] = 50 + (int)(rnd() % 150);
        sd[i] = 3 + (int)(rnd() % 8);
    }

    for(int y = 0; y < h; y++) {
        for(int x = 0; x < w; x++) {
            int r = 20, g = 22, b = 16;

            /* the grain */
            int n = (int)(rnd() % 9) - 4;
            r += n; g += n; b += n;

            /* the vignette: the edges of the screen are darker */
            int dx = x * 2 - w, dy = y * 2 - h;
            int d2 = (dx * dx) / (w * w) + (dy * dy) / (h * h);
            int v = -(d2 * 18) / 4;
            r += v; g += v; b += v;

            /* the stains */
            for(int i = 0; i < 10; i++) {
                int ax = x - sx[i], ay = y - sy[i];
                int d = ax * ax + ay * ay;
                int rr = sr[i] * sr[i];
                if(d < rr) {
                    int k = (rr - d) / rr;      /* 1 in the middle, 0 at the rim */
                    int s = sd[i] * k / 2;
                    r -= s; g -= s; b -= s;
                }
            }

            /* the cold light of the screen along the bottom */
            if(y > h * 2 / 3) {
                int t = (y - h * 2 / 3) * 16 / (h / 3 + 1);
                b += t; g += t / 3; r -= t / 4;
            }

            /* the scanlines */
            if((y % 3) == 0) { r -= 1; g -= 1; b -= 1; }

            /* the odd speck of dust */
            if((rnd() & 0x7FF) == 0) { r += 11; g += 11; b += 9; }

            r = r < 0 ? 0 : (r > 255 ? 255 : r);
            g = g < 0 ? 0 : (g > 255 ? 255 : g);
            b = b < 0 ? 0 : (b > 255 ? 255 : b);
            buf[(size_t)y * w + x] = rgb565(((uint32_t)r << 16) | ((uint32_t)g << 8) |
                                           (uint32_t)b);
        }
    }

    lv_obj_t * c = lv_canvas_create(parent);
    lv_canvas_set_buffer(c, buf, w, h, LV_COLOR_FORMAT_RGB565);
    lv_obj_set_pos(c, 0, 0);
    lv_obj_set_size(c, w, h);
    lv_obj_set_clickable(c, false);
    return c;
}

/* a crop mark: an L in the corner of the frame */
static lv_obj_t * corner_mark(lv_obj_t * parent, int x, int y, int len,
                              lv_border_side_t sides, uint32_t color)
{
    lv_obj_t * o = inert(parent);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, len, len);
    lv_obj_set_style_border_width(o, 1, LV_PART_MAIN);
    lv_obj_set_style_border_side(o, sides, LV_PART_MAIN);
    lv_obj_set_style_border_color(o, lv_color_hex(color), LV_PART_MAIN);
    return o;
}

lv_obj_t * grit_frame(lv_obj_t * parent, int x, int y, int w, int h, uint32_t color)
{
    if(w < 16 || h < 16) return NULL;

    lv_obj_t * f = inert(parent);
    lv_obj_set_pos(f, x, y);
    lv_obj_set_size(f, w, h);
    lv_obj_set_style_border_width(f, 1, LV_PART_MAIN);
    lv_obj_set_style_border_side(f, LV_BORDER_SIDE_FULL, LV_PART_MAIN);
    lv_obj_set_style_border_color(f, lv_color_hex(color), LV_PART_MAIN);

    /* the second line, a few pixels in */
    lv_obj_t * inner = inert(f);
    lv_obj_set_pos(inner, 3, 3);
    lv_obj_set_size(inner, w - 7, h - 7);
    lv_obj_set_style_border_width(inner, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(inner, lv_color_hex(PDA_FRAME_INNER), LV_PART_MAIN);
    return f;
}

void grit_corners(lv_obj_t * frame, int w, int h, uint32_t color)
{
    if(!frame) return;
    int c = 12;
    corner_mark(frame, 0, 0, c, LV_BORDER_SIDE_TOP | LV_BORDER_SIDE_LEFT, color);
    corner_mark(frame, w - c, 0, c, LV_BORDER_SIDE_TOP | LV_BORDER_SIDE_RIGHT, color);
    corner_mark(frame, 0, h - c, c, LV_BORDER_SIDE_BOTTOM | LV_BORDER_SIDE_LEFT, color);
    corner_mark(frame, w - c, h - c, c, LV_BORDER_SIDE_BOTTOM | LV_BORDER_SIDE_RIGHT, color);
}

lv_obj_t * grit_rule(lv_obj_t * parent, int x, int y, int len, bool vertical,
                     uint32_t color)
{
    lv_obj_t * o = inert(parent);
    lv_obj_set_pos(o, x, y);
    if(vertical) lv_obj_set_size(o, 1, len);
    else lv_obj_set_size(o, len, 1);
    lv_obj_set_style_bg_color(o, lv_color_hex(color), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, LV_PART_MAIN);
    return o;
}

lv_obj_t * grit_square(lv_obj_t * parent, int x, int y, int size, uint32_t color)
{
    lv_obj_t * o = inert(parent);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, size, size);
    lv_obj_set_style_bg_color(o, lv_color_hex(color), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(o, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(o, lv_color_hex(PDA_FRAME_INNER), LV_PART_MAIN);
    return o;
}
