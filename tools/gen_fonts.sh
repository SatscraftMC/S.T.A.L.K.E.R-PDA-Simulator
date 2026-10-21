#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DL="$ROOT/build/fonts-src"
OUT="$ROOT/src/assets/fonts"
mkdir -p "$DL" "$OUT"

fetch() {
    [ -f "$DL/$2" ] || curl -fsSL "$1" -o "$DL/$2"
}

fetch "https://github.com/google/fonts/raw/main/ofl/sharetechmono/ShareTechMono-Regular.ttf" "ShareTechMono-Regular.ttf"
fetch "https://github.com/google/fonts/raw/main/ofl/vt323/VT323-Regular.ttf" "VT323-Regular.ttf"
fetch "https://cdn.jsdelivr.net/npm/dseg@0.46.0/fonts/DSEG7-Classic/DSEG7Classic-Regular.ttf" "DSEG7Classic-Regular.ttf"

gen() {
    local font="$1" size="$2" name="$3" out="$4"
    pv="$(command -v lv_font_conv || true)"
    if [ -n "$pv" ]; then
        lv_font_conv --font "$DL/$font" --size "$size" --bpp 1 --format lvgl \
            --lv-include lvgl.h --no-compress -r 0x20-0x7F \
            --lv-font-name "$name" -o "$OUT/$out"
    else
        npx --yes lv_font_conv --font "$DL/$font" --size "$size" --bpp 1 --format lvgl \
            --lv-include lvgl.h --no-compress -r 0x20-0x7F \
            --lv-font-name "$name" -o "$OUT/$out"
    fi
}

gen "VT323-Regular.ttf" 20 "pda_font_title_20" "pda_font_title_20.c"
gen "ShareTechMono-Regular.ttf" 10 "pda_font_mono_10" "pda_font_mono_10.c"
gen "ShareTechMono-Regular.ttf" 14 "pda_font_mono_14" "pda_font_mono_14.c"
gen "ShareTechMono-Regular.ttf" 16 "pda_font_mono_16" "pda_font_mono_16.c"
gen "DSEG7Classic-Regular.ttf" 28 "pda_font_seg7_28" "pda_font_seg7_28.c"

echo "fonts generated in $OUT"
