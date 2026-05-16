#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 <log-file> [extra question]" >&2
    exit 1
fi

LOG_FILE="$1"
shift || true

if [[ ! -f "$LOG_FILE" ]]; then
    echo "ERROR: log file not found: $LOG_FILE" >&2
    exit 1
fi

EXTRA="${*:-Analyze the log and propose exact next steps. Do not modify files unless necessary.}"

PROMPT="/tmp/codex_analyze_log_$(date +%Y%m%d_%H%M%S).md"

cat > "$PROMPT" <<EOF_PROMPT
We are working in /root/usb-driver-fuzzing-thesis.

Analyze this log file:
$LOG_FILE

Extra request:
$EXTRA

Please:
1. Summarize whether the command succeeded.
2. Identify meaningful errors/warnings.
3. Identify whether the result is safe to commit or test further.
4. Give exact next commands.
5. Do not make changes unless the log clearly shows a bug that needs fixing.
EOF_PROMPT

echo "[codex-analyze-log] prompt: $PROMPT"
codex exec "\$(cat "$PROMPT")"
