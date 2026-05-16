#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "Usage: $0 [--uf2 <path>] [--no-serial-test] [--allow-runtime-enum-failure] [--runtime-wait-seconds <N>] <case-json> <result-name>" >&2
    echo "Example (CDC baseline): $0 experiments/cases/000_baseline_cdc.json 000_baseline_cdc_repeat_04" >&2
    echo "Example (custom HID/no-CDC): $0 --uf2 build/usb_case_custom_demo.uf2 --no-serial-test experiments/cases/001_baseline_hid_no_input.json 001_baseline_hid_no_input_01" >&2
    echo "Example (negative descriptor case): $0 --uf2 build/usb_case_custom_demo.uf2 --no-serial-test --allow-runtime-enum-failure experiments/cases/010_device_desc_blength_short.json 010_device_desc_blength_short_01" >&2
}

UF2_PATH="build/usb_case_demo.uf2"
SERIAL_TEST_ENABLED=1
ALLOW_RUNTIME_ENUM_FAILURE=0
RUNTIME_WAIT_SECONDS=8

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
        --allow-runtime-enum-failure)
            ALLOW_RUNTIME_ENUM_FAILURE=1
            shift
            ;;
        --runtime-wait-seconds)
            if [[ $# -lt 2 ]]; then
                echo "ERROR: --runtime-wait-seconds requires a value" >&2
                usage
                exit 2
            fi
            if ! [[ "$2" =~ ^[0-9]+$ ]]; then
                echo "ERROR: --runtime-wait-seconds must be an integer >= 1" >&2
                exit 2
            fi
            if [[ "$2" -lt 1 ]]; then
                echo "ERROR: --runtime-wait-seconds must be >= 1" >&2
                exit 2
            fi
            RUNTIME_WAIT_SECONDS="$2"
            shift 2
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

if [[ "$ALLOW_RUNTIME_ENUM_FAILURE" -eq 1 ]]; then
    RUNTIME_ENUM_POLICY="allow runtime enumeration failure"
else
    RUNTIME_ENUM_POLICY="require runtime enumeration"
fi

echo "Selected UF2: $UF2_PATH"
echo "Remote serial smoke-test: $SERIAL_TEST_STATUS"
echo "Runtime enumeration policy: $RUNTIME_ENUM_POLICY"
echo "Runtime wait: ${RUNTIME_WAIT_SECONDS}s"

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
sleep "$RUNTIME_WAIT_SECONDS"

echo "[9] Preparing/authorizing runtime CDC device"
if [[ "$ALLOW_RUNTIME_ENUM_FAILURE" -eq 1 ]]; then
    if ! ssh "$REMOTE_HOST" "$REMOTE_PREPARE"; then
        echo "WARN: runtime prepare failed; continuing because --allow-runtime-enum-failure is set"
    fi
else
    ssh "$REMOTE_HOST" "$REMOTE_PREPARE"
fi

echo "[10] Probing runtime enumeration status on remote host"
RUNTIME_ENUM_STATUS="$(
    ssh "$REMOTE_HOST" "bash -s" <<'EOF'
set -euo pipefail
TTY_COUNT="$(compgen -G '/dev/ttyACM*' | wc -l || true)"
if [[ "$TTY_COUNT" -ge 1 ]]; then
    echo "normal"
    exit 0
fi
if lsusb | awk '{print tolower($6)}' | grep -qE '^[0-9a-f]{4}:[0-9a-f]{4}$'; then
    echo "partial"
else
    echo "failed"
fi
EOF
)"
echo "Runtime USB enumeration status: $RUNTIME_ENUM_STATUS"

if [[ "$ALLOW_RUNTIME_ENUM_FAILURE" -eq 0 && "$RUNTIME_ENUM_STATUS" == "failed" ]]; then
    echo "ERROR: runtime USB enumeration failed (use --allow-runtime-enum-failure for negative descriptor cases)" >&2
    exit 1
fi

if [[ "$SERIAL_TEST_ENABLED" -eq 1 ]]; then
    if [[ "$ALLOW_RUNTIME_ENUM_FAILURE" -eq 1 ]]; then
        if [[ "$RUNTIME_ENUM_STATUS" == "normal" ]]; then
            echo "[11] Running remote serial smoke-test"
            ssh "$REMOTE_HOST" "~/vkr-usb/scripts/test_usb_case_serial_linux.py"
        else
            echo "[11] Skipping remote serial smoke-test (runtime enumeration is $RUNTIME_ENUM_STATUS)"
        fi
    else
        echo "[11] Running remote serial smoke-test"
        ssh "$REMOTE_HOST" "~/vkr-usb/scripts/test_usb_case_serial_linux.py"
    fi
else
    echo "[11] Skipping remote serial smoke-test (--no-serial-test)"
fi

echo "[12] Capturing Linux USB snapshot: $RESULT_NAME"
SNAPSHOT_OK=1
if ! ssh "$REMOTE_HOST" "~/vkr-usb/scripts/capture_linux_usb_snapshot.sh '$RESULT_NAME'"; then
    SNAPSHOT_OK=0
    echo "WARN: primary remote snapshot script failed"
fi

echo "[13] Capturing fallback runtime logs on remote host"
FALLBACK_LOG_DIR="$(
    ssh "$REMOTE_HOST" "bash -s -- '$RESULT_NAME' '$RUNTIME_ENUM_STATUS'" <<'EOF'
set -euo pipefail
RESULT_NAME="$1"
RUNTIME_ENUM_STATUS="$2"
TS="$(date +%Y%m%d_%H%M%S)"
LOG_DIR="$HOME/vkr-usb/logs/${RESULT_NAME}_${TS}"
mkdir -p "$LOG_DIR"

{
    echo "result_name=$RESULT_NAME"
    echo "timestamp=$TS"
    echo "runtime_enum_status=$RUNTIME_ENUM_STATUS"
    echo "host=$(hostname)"
    echo "kernel=$(uname -r)"
} > "$LOG_DIR/runtime_status.txt"

lsusb > "$LOG_DIR/lsusb.txt" 2>&1 || true
lsusb -t > "$LOG_DIR/lsusb_tree.txt" 2>&1 || true
journalctl -k -n 120 --no-pager > "$LOG_DIR/journalctl_k_tail.txt" 2>&1 || true
dmesg | tail -n 120 > "$LOG_DIR/dmesg_tail.txt" 2>&1 || true
ls -l /dev/ttyACM* /dev/ttyUSB* > "$LOG_DIR/tty_devices.txt" 2>&1 || true
find /sys/bus/usb/devices -maxdepth 2 -type f \
    \( -name idVendor -o -name idProduct -o -name manufacturer -o -name product -o -name serial \) \
    -print -exec cat {} \; > "$LOG_DIR/sys_bus_usb_devices_summary.txt" 2>&1 || true

echo "$LOG_DIR"
EOF
)"

echo "Fallback logs directory: $FALLBACK_LOG_DIR"

if [[ "$SNAPSHOT_OK" -eq 0 && "$ALLOW_RUNTIME_ENUM_FAILURE" -eq 0 ]]; then
    echo "ERROR: primary remote snapshot failed (fallback logs were captured, but strict mode requires snapshot success)" >&2
    exit 1
fi

echo "[14] Done."
echo "Remote snapshot directory prefix: ~/vkr-usb/logs/${RESULT_NAME}_<timestamp>"
