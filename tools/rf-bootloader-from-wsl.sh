#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

POWERSHELL_EXE="/mnt/c/Windows/System32/WindowsPowerShell/v1.0/powershell.exe"

if [[ ! -x "$POWERSHELL_EXE" ]]; then
    echo "ERROR: PowerShell not found at $POWERSHELL_EXE" >&2
    exit 1
fi

PY_SCRIPT_UNC="\\\\wsl.localhost\\ubuntu${REPO_DIR//\//\\}\\firmware\\esp32c3_bridge\\esp32_send_bootloader_safe.py"

echo "[1] Sending ESP32 RF bootloader command via Windows COM3..."
echo "[2] Python script: $PY_SCRIPT_UNC"

"$POWERSHELL_EXE" \
    -NoProfile \
    -ExecutionPolicy Bypass \
    -Command "py '$PY_SCRIPT_UNC'"

echo "[3] ESP32 command finished."
