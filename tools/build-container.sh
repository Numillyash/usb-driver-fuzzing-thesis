#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$ROOT_DIR"

docker compose run --rm rp2040-dev bash -lc '
  cmake -S firmware -B build -G Ninja \
    -DPICO_BOARD=waveshare_rp2040_zero \
    -DCMAKE_BUILD_TYPE=Release && \
  cmake --build build -j"$(nproc)"
'

echo
echo "Build complete: $ROOT_DIR/build/portable_demo.uf2"
