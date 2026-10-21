#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD="$ROOT/build"

cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD" -j"$(nproc)"

echo
echo "Built: $BUILD/pda"
echo "Run:   $BUILD/pda"
