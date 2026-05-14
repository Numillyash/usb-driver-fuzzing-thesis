#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 2 ]]; then
    echo "Usage: $0 <case-json> <result-name>" >&2
    echo "Example: $0 experiments/cases/000_baseline_cdc.json 000_baseline_cdc_repeat_04" >&2
    exit 2
fi

CASE_JSON="$1"
RESULT_NAME="$2"

REMOTE_HOST="${REMOTE_HOST:-vkr-linux}"
REMOTE_INCOMING="${REMOTE_INCOMING:-~/vkr-usb/incoming}"
REMOTE_UF2="${REMOTE_UF2:-~/vkr-usb/incoming/usb_case_demo.uf2}"
REMOTE_PREPARE="${REMOTE_PREPARE:-sudo -n /usr/local/sbin/vkr-rp2040-usb-prepare}"

if [[ ! -f "$CASE_JSON" ]]; then
    echo "ERROR: case JSON not found: $CASE_JSON" >&2
    exit 1
fi

echo "[1] Generating USB case config from: $CASE_JSON"
python3 tools/gen_usb_case_config.py "$CASE_JSON"

echo "[2] Building RP2040 firmware"
./tools/build-container.sh

if [[ ! -f build/usb_case_demo.uf2 ]]; then
    echo "ERROR: build/usb_case_demo.uf2 not found after build" >&2
    exit 1
fi

echo "[3] Copying UF2 to remote Linux host"
ssh "$REMOTE_HOST" "mkdir -p $REMOTE_INCOMING"
scp build/usb_case_demo.uf2 "$REMOTE_HOST:~/vkr-usb/incoming/usb_case_demo.uf2"

echo "[4] Sending RF bootloader command through ESP32"
./tools/rf-bootloader-from-wsl.sh

echo "[5] Waiting for RP2040 BOOTSEL enumeration"
sleep 2

echo "[6] Preparing/authorizing RP2040 USB device on remote host"
ssh "$REMOTE_HOST" "$REMOTE_PREPARE"

echo "[7] Flashing RP2040 UF2 on remote host"
ssh "$REMOTE_HOST" "~/vkr-usb/scripts/flash_rp2040_uf2.sh $REMOTE_UF2"

echo "[8] Waiting for RP2040 runtime enumeration"
sleep 2

echo "[9] Preparing/authorizing runtime CDC device"
ssh "$REMOTE_HOST" "$REMOTE_PREPARE"

echo "[10] Running remote serial smoke-test"
ssh "$REMOTE_HOST" "~/vkr-usb/scripts/test_usb_case_serial_linux.py"

echo "[11] Capturing Linux USB snapshot: $RESULT_NAME"
ssh "$REMOTE_HOST" "~/vkr-usb/scripts/capture_linux_usb_snapshot.sh '$RESULT_NAME'"

echo "[12] Done."
echo "Remote snapshot directory prefix: ~/vkr-usb/logs/${RESULT_NAME}_<timestamp>"
