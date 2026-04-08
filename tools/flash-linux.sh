#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
UF2="${1:-$ROOT_DIR/build/portable_demo.uf2}"

if [[ ! -f "$UF2" ]]; then
  echo "UF2 file not found: $UF2" >&2
  exit 1
fi

for base in "/media/$USER" "/run/media/$USER" "/mnt"; do
  if [[ -d "$base/RPI-RP2" ]]; then
    cp "$UF2" "$base/RPI-RP2/"
    sync
    echo "UF2 copied to $base/RPI-RP2/"
    exit 0
  fi
done

echo "RPI-RP2 mount point not found. Put the board into BOOTSEL mode first." >&2
exit 1
