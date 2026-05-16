# Execution logs

This directory stores curated execution logs for reproducible USB robustness experiments.

Tracked:
- `*.log` — wrapper logs from `tools/run-with-log.sh`;
- `*.quick_analysis.txt` — compact grep/evidence summaries for a run.

Not tracked:
- temporary prompts;
- ad-hoc debug dumps;
- unrelated local logs.

Do not add the whole `logs/` tree. Stage only relevant run logs for cases that are referenced from `docs/results/` or `results/raw/`.
