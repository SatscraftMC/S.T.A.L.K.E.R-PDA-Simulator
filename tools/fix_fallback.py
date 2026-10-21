#!/usr/bin/env python3
"""Wire a fallback font into a generated font.

Share Tech Mono has no Polish letters (apart from "ó" and "ł"), so the
characters it misses come from VT323 - the font the titles use. The font
struct is const, so the fallback has to be set in the initializer.
"""
import sys

path, fb = sys.argv[1], sys.argv[2]
s = open(path).read()
if '.fallback = &%s,' % fb in s:
    sys.exit(0)
s = s.replace('.fallback = NULL,', '.fallback = &%s,' % fb, 1)
s = s.replace('#include "lvgl.h"', '#include "lvgl.h"\nextern const lv_font_t %s;' % fb, 1)
open(path, 'w').write(s)
