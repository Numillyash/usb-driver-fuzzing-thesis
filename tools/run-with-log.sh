#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 2 ]]; then
    echo "Usage: $0 <log-name> <command...>" >&2
    echo "Example: $0 build_010 ./tools/build-container.sh" >&2
    exit 1
fi

LOG_NAME="$1"
shift

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LOG_DIR="$REPO_DIR/logs/runs"
mkdir -p "$LOG_DIR"

TS="$(date +%Y%m%d_%H%M%S)"
LOG="$LOG_DIR/${TS}_${LOG_NAME}.log"

echo "[run-with-log] repo: $REPO_DIR"
echo "[run-with-log] log:  $LOG"
echo "[run-with-log] cmd:  $*"
echo

set -o pipefail
{
    echo "===== timestamp ====="
    date -Is
    echo
    echo "===== cwd ====="
    pwd
    echo
    echo "===== command ====="
    printf '%q ' "$@"
    echo
    echo
    echo "===== output ====="
    "$@"
} 2>&1 | tee "$LOG"

echo
echo "[run-with-log] saved: $LOG"
