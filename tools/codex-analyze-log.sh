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

EXTRA="${*:-Analyze the run with focus on USB robustness outcomes and next reproducible actions. Do not modify files unless necessary.}"

LOG_BASENAME="$(basename "$LOG_FILE")"
if [[ "$LOG_BASENAME" =~ ^[0-9]{8}_[0-9]{6}_(.+)\.log$ ]]; then
    RUN_NAME="${BASH_REMATCH[1]}"
else
    echo "ERROR: cannot derive RUN_NAME from log file name: $LOG_BASENAME" >&2
    echo "Expected format: logs/runs/YYYYMMDD_HHMMSS_<run_name>.log" >&2
    exit 1
fi

CASE_JSON=""
if CASE_JSON_LINE="$(rg -n --no-heading -m1 'experiments/cases/[^[:space:]]+\.json' "$LOG_FILE" || true)"; then
    if [[ -n "$CASE_JSON_LINE" ]]; then
        CASE_JSON="$(sed -E 's#.*(experiments/cases/[^[:space:]]+\.json).*#\1#' <<<"$CASE_JSON_LINE")"
    fi
fi

RESULT_DIRS=()
while IFS= read -r path; do
    [[ -n "$path" ]] && RESULT_DIRS+=("$path")
done < <(find results/raw/linux -maxdepth 1 -mindepth 1 -type d -name "${RUN_NAME}_*" 2>/dev/null | sort)

QUICK_ANALYSIS_FILES=()
while IFS= read -r path; do
    [[ -n "$path" ]] && QUICK_ANALYSIS_FILES+=("$path")
done < <(find logs/runs -maxdepth 1 -type f -name "*${RUN_NAME}*.quick_analysis.txt" 2>/dev/null | sort)

PROMPT="/tmp/codex_analyze_log_${RUN_NAME}_$(date +%Y%m%d_%H%M%S).md"

{
    echo "We are working in /root/usb-driver-fuzzing-thesis."
    echo
    echo "Analyze this USB robustness run."
    echo
    echo "Log file: $LOG_FILE"
    echo "Run name: $RUN_NAME"

    if [[ -n "$CASE_JSON" ]]; then
        echo "Case JSON (detected from wrapper log): $CASE_JSON"
    else
        echo "Case JSON (detected from wrapper log): not found"
    fi

    if ((${#RESULT_DIRS[@]} > 0)); then
        echo "Local result directories:"
        for d in "${RESULT_DIRS[@]}"; do
            echo "- $d"
        done
    else
        echo "Local result directories: none found for pattern results/raw/linux/${RUN_NAME}_*"
    fi

    if ((${#QUICK_ANALYSIS_FILES[@]} > 0)); then
        echo "Quick analysis files:"
        for f in "${QUICK_ANALYSIS_FILES[@]}"; do
            echo "- $f"
        done
    else
        echo "Quick analysis files: none found"
    fi

    echo
    echo "Extra request:"
    echo "$EXTRA"
    echo
    echo "Important: if evidence is insufficient, explicitly ask to inspect the actual files listed above before concluding."
    echo
    echo "=== wrapper log tail (last 160 lines) ==="
    tail -n 160 "$LOG_FILE" || true

    echo
    echo "=== wrapper grep evidence ==="
    rg -n -i 'runtime usb enumeration status|partial|reject|error|fail|unable|descriptor|enumerat|idvendor|idproduct|tty|hidraw|block|panic|oops|bug:' "$LOG_FILE" || true

    for dir in "${RESULT_DIRS[@]}"; do
        echo
        echo "=== result dir: $dir ==="
        find "$dir" -maxdepth 2 -type f -print | sort

        echo
        echo "--- grep evidence from $dir ---"
        rg -n -i 'usb|descriptor|enumerat|idvendor|idproduct|cdc_acm|ttyacm|hidraw|hid-generic|mass storage|scsi|blk|block|error|warn|fail|unable|stall|timeout|reset|disconnect|panic|oops|bug:' "$dir" || true

        echo
        echo "--- tail snippets from $dir ---"
        for f in "$dir"/system_snapshot.txt "$dir"/runtime_status.txt "$dir"/journalctl_k_tail_300.txt "$dir"/journalctl_k_tail.txt "$dir"/dmesg_tail_300.txt "$dir"/dmesg_tail.txt; do
            if [[ -f "$f" ]]; then
                echo "### tail: $f"
                tail -n 120 "$f" || true
            fi
        done
    done

    if ((${#QUICK_ANALYSIS_FILES[@]} > 0)); then
        for qa in "${QUICK_ANALYSIS_FILES[@]}"; do
            echo
            echo "=== quick analysis excerpt: $qa (head 80 + tail 80) ==="
            sed -n '1,80p' "$qa" || true
            echo
            echo "--- tail ---"
            tail -n 80 "$qa" || true
        done
    fi

    echo
    echo "Please provide:"
    echo "1. Classification (OK / EXPECTED_REJECT / PARTIAL_ENUM / DRIVER_BIND_ERROR / USERSPACE_FAILURE / KERNEL_WARNING / DRIVER_CRASH / SYSTEM_CRASH)."
    echo "2. Strict evidence list with file paths and line snippets."
    echo "3. Confidence level and evidence gaps."
    echo "4. Exact next commands for deeper verification."
    echo "5. No code edits unless a concrete script bug is proven."
} > "$PROMPT"

echo "[codex-analyze-log] prompt: $PROMPT"
codex exec "Read the generated USB run-analysis prompt from this file: $PROMPT

Do not just print the prompt file. Inspect the referenced repository files, wrapper log, quick-analysis file, and results/raw/linux directories. Then perform the requested analysis.

If the prompt asks for documentation updates or a git commit, make only the requested safe changes. Do not run hardware experiments. Do not flash devices. Do not add BadUSB behavior, keyboard injection, host command execution, persistence, credential access, reverse shells, UAC bypass, or privilege escalation."
