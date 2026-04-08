#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
UF2="$ROOT_DIR/build/portable_demo.uf2"
PS1="$ROOT_DIR/tools/flash-windows.ps1"

if [[ ! -f "$UF2" ]]; then
  echo "UF2 file not found: $UF2" >&2
  exit 1
fi

powershell.exe -ExecutionPolicy Bypass -File "$(wslpath -w "$PS1")" -Uf2Path "$(wslpath -w "$UF2")"
