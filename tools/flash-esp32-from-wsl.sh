#!/usr/bin/env bash
set -euo pipefail

PORT="${1:-COM3}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
ESP_DIR="$REPO_DIR/firmware/esp32c3_bridge"

POWERSHELL_EXE="/mnt/c/Windows/System32/WindowsPowerShell/v1.0/powershell.exe"

if [[ ! -x "$POWERSHELL_EXE" ]]; then
    echo "ERROR: PowerShell not found at $POWERSHELL_EXE" >&2
    exit 1
fi

ESP_DIR_UNC="\\\\wsl.localhost\\ubuntu${ESP_DIR//\//\\}"

echo "[1] Flashing ESP32-C3 from WSL via Windows esptool"
echo "[2] ESP project: $ESP_DIR_UNC"
echo "[3] Port: $PORT"

"$POWERSHELL_EXE" \
    -NoProfile \
    -ExecutionPolicy Bypass \
    -Command "Set-Location '$ESP_DIR_UNC'; py -m esptool --chip esp32c3 -p '$PORT' -b 460800 --before default-reset --after hard-reset write-flash --flash-mode dio --flash-freq 80m --flash-size 4MB 0x0 build/bootloader/bootloader.bin 0x8000 build/partition_table/partition-table.bin 0x10000 build/esp32c3_bridge.bin"

echo "[4] ESP32-C3 flash finished."
