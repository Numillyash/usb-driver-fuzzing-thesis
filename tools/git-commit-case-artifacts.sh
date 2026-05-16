#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
    echo "Usage: $0 <run-name> <commit-message>" >&2
    exit 1
fi

RUN_NAME="$1"
COMMIT_MSG="$2"

case "$RUN_NAME" in
    [0-9][0-9][0-9]_*)
        ;;
    *)
        echo "ERROR: suspicious run name: $RUN_NAME" >&2
        exit 1
        ;;
esac

git add "docs/results/${RUN_NAME}.md" 2>/dev/null || true
git add -f "logs/runs/"*"${RUN_NAME}"*.log 2>/dev/null || true
git add -f "logs/runs/"*"${RUN_NAME}"*.quick_analysis.txt 2>/dev/null || true
git add -f "results/raw/linux/${RUN_NAME}_"* 2>/dev/null || true

echo "=== staged files ==="
git diff --cached --name-only

if git diff --cached --quiet; then
    echo "Nothing staged."
    exit 0
fi

git commit -m "$COMMIT_MSG"
git push
