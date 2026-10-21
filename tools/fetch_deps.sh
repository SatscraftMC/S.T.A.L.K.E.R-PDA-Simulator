#!/usr/bin/env bash
# One-time setup after cloning:
#   1. fetch the vendored LVGL (kept out of git - 294MB)
#   2. reassemble the map levels that are stored in git as split parts
#      (GitHub's 100MB per-file limit; the two biggest levels are split)
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LVGL_VER="v9.6.0"
LVGL_DIR="$ROOT/third_party/lvgl"

if [ -d "$LVGL_DIR/.git" ]; then
    echo "LVGL already present: $LVGL_DIR"
else
    echo "fetching LVGL $LVGL_VER ..."
    mkdir -p "$ROOT/third_party"
    git clone --depth 1 --branch "$LVGL_VER" https://github.com/lvgl/lvgl.git "$LVGL_DIR"
fi

# --- reassemble split map levels ---
reassemble() {
    local dir="$1" base="$2"
    local parts=("$dir/$base".part*)
    [ -e "${parts[0]}" ] || return 0
    if [ -f "$dir/$base" ]; then
        echo "map level already assembled: $dir/$base"
        return 0
    fi
    echo "assembling $dir/$base from ${#parts[@]} parts ..."
    cat "${parts[@]}" > "$dir/$base"
    echo "  -> $(du -h "$dir/$base" | cut -f1)"
}

reassemble "$ROOT/assets/maps/madeira-island" "z14.rgb565"
reassemble "$ROOT/assets/maps/funchal"         "z17.rgb565"

cat <<'EOF'

build:  ./build.sh
run:    ./build/pda

add a new region / deeper level for an existing one:
  python3 tools/fetch_map.py --name madeira --lon0 -17.35 --lon1 -16.15 \
                              --lat0 33.12 --lat1 32.30 --zooms 12,13

check the map:
  python3 tools/analyze_map.py
EOF
