#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 3 || $# -gt 4 ]]; then
    echo "Usage: $0 <run-name> <case-json> <mutation-description> [commit-message]" >&2
    exit 1
fi

RUN_NAME="$1"
CASE_JSON="$2"
MUTATION_DESC="$3"
COMMIT_MSG="${4:-Add Linux result artifacts for ${RUN_NAME}}"

UF2="${UF2:-build/usb_case_custom_demo.uf2}"

if [[ ! -f "$CASE_JSON" ]]; then
    echo "ERROR: case JSON not found: $CASE_JSON" >&2
    exit 1
fi

if [[ ! -f "$UF2" ]]; then
    echo "ERROR: UF2 not found: $UF2" >&2
    exit 1
fi

echo "=== [0] Run metadata ==="
echo "RUN_NAME=$RUN_NAME"
echo "CASE_JSON=$CASE_JSON"
echo "MUTATION_DESC=$MUTATION_DESC"
echo "UF2=$UF2"
echo "COMMIT_MSG=$COMMIT_MSG"

echo
echo "=== [1] Run remote Linux case with local execution log ==="
./tools/run-with-log.sh "$RUN_NAME" \
  ./tools/run-remote-linux-case.sh \
  --uf2 "$UF2" \
  --no-serial-test \
  --allow-runtime-enum-failure \
  "$CASE_JSON" \
  "$RUN_NAME"

echo
echo "=== [2] Locate wrapper log ==="
LOG="$(find logs/runs -type f -name "*${RUN_NAME}.log" -printf '%T@ %p\n' | sort -n | tail -1 | cut -d' ' -f2-)"
if [[ -z "$LOG" || ! -f "$LOG" ]]; then
    echo "ERROR: wrapper log not found for $RUN_NAME" >&2
    exit 1
fi
echo "LOG=$LOG"
tail -120 "$LOG" || true

echo
echo "=== [3] Copy remote Linux artifacts ==="
mkdir -p results/raw/linux

ssh vkr-linux "find ~/vkr-usb/logs -maxdepth 1 -type d -name '${RUN_NAME}_*' -print | sort"

scp -r vkr-linux:~/vkr-usb/logs/${RUN_NAME}_* results/raw/linux/

echo
echo "=== [4] Create quick analysis ==="
ANALYSIS_TXT="$(dirname "$LOG")/$(basename "$LOG" .log).quick_analysis.txt"

{
  echo "=== wrapper runtime status ==="
  grep -nE "Runtime USB enumeration status|allow runtime|Skipping remote serial|Fallback logs directory|snapshot|Done" "$LOG" || true

  echo
  echo "=== visible usb ids in snapshot ==="
  grep -RniE "2e8a|0000|RP2040|Raspberry|Pico|usb_case" \
    results/raw/linux/${RUN_NAME}_* || true

  echo
  echo "=== kernel usb/errors ==="
  grep -RniE "descriptor|device descriptor|configuration|config|wTotalLength|bNumInterfaces|interface|endpoint|wMaxPacketSize|direction|class|subclass|protocol|malformed|invalid|error|unable|failed|not accepting|enumerat|read/64|usb 1-|reset|disconnect|stall|timeout|panic|oops|bug:" \
    results/raw/linux/${RUN_NAME}_* || true

  echo
  echo "=== tty/hid/block evidence ==="
  grep -RniE "ttyACM|ttyUSB|hidraw|hid-generic|cdc_acm|Mass Storage|usb-storage|block|sda|sdb|RPI-RP2" \
    results/raw/linux/${RUN_NAME}_* || true
} | tee "$ANALYSIS_TXT"

echo "ANALYSIS_TXT=$ANALYSIS_TXT"

echo
echo "=== [5] Run Codex analyzer ==="
./tools/codex-analyze-log.sh "$LOG" \
  "Analyze this USB robustness case. Mutation: ${MUTATION_DESC}. Use logs/runs quick analysis and results/raw/linux evidence. Classify the result using OK/EXPECTED_REJECT/PARTIAL_ENUM/DRIVER_BIND_ERROR/USERSPACE_FAILURE/KERNEL_WARNING/DRIVER_CRASH/SYSTEM_CRASH. Be strict: do not overclaim a kernel bug, driver crash, or system crash unless evidence proves it. If evidence is insufficient, say exactly what is missing. Do not add unsafe behavior."

echo
echo "=== [6] Ask Codex to create docs/results entry ==="
DOC_TASK="/tmp/vkr_codex_document_${RUN_NAME}.md"

cat > "$DOC_TASK" <<EOF_DOC
We are working in /root/usb-driver-fuzzing-thesis.

Document the completed USB robustness run:

RUN_NAME=${RUN_NAME}
CASE=${CASE_JSON}
MUTATION=${MUTATION_DESC}

Use:
- logs/runs/*${RUN_NAME}*.log
- logs/runs/*${RUN_NAME}*.quick_analysis.txt
- results/raw/linux/${RUN_NAME}_*

Create:
- docs/results/${RUN_NAME}.md

The document must include:
1. Case purpose.
2. Input artifacts.
3. Observed runtime enumeration result.
4. Linux evidence from wrapper log, quick analysis, and raw result directories.
5. Classification using:
   OK, EXPECTED_REJECT, PARTIAL_ENUM, DRIVER_BIND_ERROR, USERSPACE_FAILURE, KERNEL_WARNING, DRIVER_CRASH, SYSTEM_CRASH.
6. Confidence level.
7. Evidence gaps.
8. Next verification commands.

Be strict:
- do not overclaim kernel bugs;
- do not claim driver crash unless logs prove it;
- do not claim system crash unless there is explicit evidence;
- if Linux accepts the malformed case without crash, classify accordingly;
- if Linux rejects or partially enumerates it, classify accordingly.

Do not run hardware experiments.
Do not flash devices.
Do not add unsafe USB behavior.
Do not modify firmware/code.
EOF_DOC

codex exec "Read and execute ${DOC_TASK}. Inspect the repository files and create/update only docs/results/${RUN_NAME}.md."

echo
echo "=== [7] Validate docs result ==="
if [[ ! -f "docs/results/${RUN_NAME}.md" ]]; then
    echo "ERROR: docs/results/${RUN_NAME}.md was not created" >&2
    exit 1
fi

bash -n tools/codex-analyze-log.sh
bash -n tools/git-commit-case-artifacts.sh

echo
echo "=== [8] Commit and push via safe helper ==="
./tools/git-commit-case-artifacts.sh "$RUN_NAME" "$COMMIT_MSG"

echo
echo "=== [9] Final status ==="
git status --short
git log --oneline --decorate -8
