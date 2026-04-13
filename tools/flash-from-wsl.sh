#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PS_SCRIPT_UNC="\\\\wsl.localhost\\ubuntu${SCRIPT_DIR//\//\\}\\flash-windows.ps1"

POWERSHELL_EXE="/mnt/c/Windows/System32/WindowsPowerShell/v1.0/powershell.exe"

if [[ ! -x "$POWERSHELL_EXE" ]]; then
  echo "PowerShell not found at: $POWERSHELL_EXE" >&2
  exit 1
fi

"$POWERSHELL_EXE" -ExecutionPolicy Bypass -File "$PS_SCRIPT_UNC"