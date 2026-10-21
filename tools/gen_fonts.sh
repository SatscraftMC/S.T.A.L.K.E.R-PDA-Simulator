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
    local font="$1" size="$2" name="$3" out="$4" range="${5:-0x20-0x7F}"
    pv="$(command -v lv_font_conv || true)"
    if [ -n "$pv" ]; then
        lv_font_conv --font "$DL/$font" --size "$size" --bpp 1 --format lvgl \
            --lv-include lvgl.h --no-compress -r "$range" \
            --lv-font-name "$name" -o "$OUT/$out"
    else
        npx --yes lv_font_conv --font "$DL/$font" --size "$size" --bpp 1 --format lvgl \
            --lv-include lvgl.h --no-compress -r "$range" \
            --lv-font-name "$name" -o "$OUT/$out"
    fi
}

# the interface font: the Latin-1 and the Latin Extended-A range, so the
# Polish letters are in there
for size in 10 14 16; do
    gen "ShareTechMono-Regular.ttf" "$size" "pda_font_mono_$size" "pda_font_mono_$size.c" "0x20-0x17F"
done

# the title font, and the fallbacks for the characters Share Tech Mono misses
gen "VT323-Regular.ttf" 20 "pda_font_title_20" "pda_font_title_20.c" "0x20-0x17F"
for size in 10 14 16; do
    gen "VT323-Regular.ttf" "$size" "pda_font_fallback_$size" "pda_font_fallback_$size.c" "0x100-0x17F"
done

for size in 10 14 16; do
    python3 "$ROOT/tools/fix_fallback.py" "$OUT/pda_font_mono_$size.c" "pda_font_fallback_$size"
done

# the clock: digits only
gen "DSEG7Classic-Regular.ttf" 28 "pda_font_seg7_28" "pda_font_seg7_28.c" "0x20-0x7F"

echo "fonts generated in $OUT"
