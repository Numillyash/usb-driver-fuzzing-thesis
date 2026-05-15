#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "Usage: $0 [--uf2 <path>] [--no-serial-test] <case-json> <result-name>" >&2
    echo "Example (CDC baseline): $0 experiments/cases/000_baseline_cdc.json 000_baseline_cdc_repeat_04" >&2
    echo "Example (custom HID/no-CDC): $0 --uf2 build/usb_case_custom_demo.uf2 --no-serial-test experiments/cases/001_baseline_hid_no_input.json 001_baseline_hid_no_input_01" >&2
}

UF2_PATH="build/usb_case_demo.uf2"
SERIAL_TEST_ENABLED=1

while [[ $# -gt 0 ]]; do
    case "$1" in
        --uf2)
            if [[ $# -lt 2 ]]; then
                echo "ERROR: --uf2 requires a value" >&2
                usage
                exit 2
            fi
            UF2_PATH="$2"
            shift 2
            ;;
        --no-serial-test)
            SERIAL_TEST_ENABLED=0
            shift
            ;;
        --help|-h)
            usage
            exit 0
            ;;
        --*)
            echo "ERROR: unknown option: $1" >&2
            usage
            exit 2
            ;;
        *)
            break
            ;;
    esac
done

if [[ $# -lt 2 ]]; then
    usage
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

if [[ ! -f "$UF2_PATH" ]]; then
    echo "ERROR: UF2 not found after build: $UF2_PATH" >&2
    exit 1
fi

if [[ "$SERIAL_TEST_ENABLED" -eq 1 ]]; then
    SERIAL_TEST_STATUS="enabled"
else
    SERIAL_TEST_STATUS="disabled"
fi

echo "Selected UF2: $UF2_PATH"
echo "Remote serial smoke-test: $SERIAL_TEST_STATUS"

echo "[3] Copying UF2 to remote Linux host"
ssh "$REMOTE_HOST" "mkdir -p $REMOTE_INCOMING"
scp "$UF2_PATH" "$REMOTE_HOST:~/vkr-usb/incoming/usb_case_demo.uf2"

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

if [[ "$SERIAL_TEST_ENABLED" -eq 1 ]]; then
    echo "[10] Running remote serial smoke-test"
    ssh "$REMOTE_HOST" "~/vkr-usb/scripts/test_usb_case_serial_linux.py"
else
    echo "[10] Skipping remote serial smoke-test (--no-serial-test)"
fi

echo "[11] Capturing Linux USB snapshot: $RESULT_NAME"
ssh "$REMOTE_HOST" "~/vkr-usb/scripts/capture_linux_usb_snapshot.sh '$RESULT_NAME'"

echo "[12] Done."
echo "Remote snapshot directory prefix: ~/vkr-usb/logs/${RESULT_NAME}_<timestamp>"
