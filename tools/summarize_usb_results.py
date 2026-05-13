#!/usr/bin/env python3
"""Сводка результатов USB-экспериментов из experiments/results/summary.csv."""

from __future__ import annotations

import argparse
import csv
from collections import Counter
from pathlib import Path
from typing import Dict, Iterable, List, Sequence

SUMMARY_PATH = Path("experiments/results/summary.csv")
REPORT_PATH = Path("experiments/results/summary_report.md")

REQUIRED_COLUMNS: Sequence[str] = (
    "case_id",
    "case_name",
    "group",
    "base_persona",
    "mutation_summary",
    "os",
    "enumeration_result",
    "driver_result",
    "log_artifacts",
    "trace_artifacts",
    "stability_impact",
    "classification",
    "notes",
)

ALLOWED_CLASSIFICATIONS = {
    "normal_handling",
    "enumeration_rejected",
    "descriptor_parse_error",
    "driver_error",
    "device_side_failure",
    "host_side_instability",
    "inconclusive",
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Проверка experiments/results/summary.csv и вывод статистики "
            "по ОС и классификациям."
        )
    )
    parser.add_argument(
        "--write-report",
        action="store_true",
        help=(
            "Сохранить Markdown-отчёт в "
            "experiments/results/summary_report.md"
        ),
    )
    return parser.parse_args()


def read_rows(path: Path) -> List[Dict[str, str]]:
    with path.open("r", encoding="utf-8", newline="") as file:
        reader = csv.DictReader(file)
        if reader.fieldnames is None:
            raise ValueError("Файл summary.csv пустой или повреждён: отсутствует заголовок.")

        missing_columns = [name for name in REQUIRED_COLUMNS if name not in reader.fieldnames]
        if missing_columns:
            raise ValueError(
                "В summary.csv отсутствуют обязательные столбцы: "
                + ", ".join(missing_columns)
            )

        rows: List[Dict[str, str]] = []
        for raw_row in reader:
            row = {key: (value or "").strip() for key, value in raw_row.items()}
            rows.append(row)

    return rows


def validate_classifications(rows: Iterable[Dict[str, str]]) -> List[str]:
    errors: List[str] = []
    for index, row in enumerate(rows, start=2):
        classification = row.get("classification", "")
        if classification not in ALLOWED_CLASSIFICATIONS:
            errors.append(
                f"Строка {index}: недопустимое значение classification={classification!r}."
            )
    return errors


def build_counters(rows: Iterable[Dict[str, str]]) -> tuple[Counter[str], Counter[str]]:
    os_counter: Counter[str] = Counter()
    class_counter: Counter[str] = Counter()

    for row in rows:
        os_name = row.get("os", "") or "<unspecified>"
        classification = row.get("classification", "") or "<unspecified>"
        os_counter[os_name] += 1
        class_counter[classification] += 1

    return os_counter, class_counter


def format_counter(title: str, counter: Counter[str]) -> str:
    lines = [title]
    if not counter:
        lines.append("  (нет данных)")
        return "\n".join(lines)

    for key, value in sorted(counter.items(), key=lambda item: (-item[1], item[0])):
        lines.append(f"  - {key}: {value}")
    return "\n".join(lines)


def generate_markdown_report(
    rows_count: int,
    os_counter: Counter[str],
    class_counter: Counter[str],
) -> str:
    lines: List[str] = []
    lines.append("# Сводка результатов USB-экспериментов")
    lines.append("")
    lines.append(f"Всего записей: **{rows_count}**")
    lines.append("")
    lines.append("## Распределение по ОС")
    lines.append("")
    if os_counter:
        for os_name, count in sorted(os_counter.items(), key=lambda item: (-item[1], item[0])):
            lines.append(f"- `{os_name}`: {count}")
    else:
        lines.append("- Нет данных")

    lines.append("")
    lines.append("## Распределение по classification")
    lines.append("")
    if class_counter:
        for cls, count in sorted(class_counter.items(), key=lambda item: (-item[1], item[0])):
            lines.append(f"- `{cls}`: {count}")
    else:
        lines.append("- Нет данных")

    lines.append("")
    lines.append("## Допустимые classification")
    lines.append("")
    for value in sorted(ALLOWED_CLASSIFICATIONS):
        lines.append(f"- `{value}`")

    return "\n".join(lines) + "\n"


def main() -> int:
    args = parse_args()

    if not SUMMARY_PATH.exists():
        print("Файл experiments/results/summary.csv не найден.")
        print("Создайте его на основе шаблона experiments/results/summary_template.csv.")
        print("Пример:")
        print(
            "  cp experiments/results/summary_template.csv "
            "experiments/results/summary.csv"
        )
        return 0

    try:
        rows = read_rows(SUMMARY_PATH)
    except ValueError as error:
        print(f"Ошибка формата summary.csv: {error}")
        return 1

    validation_errors = validate_classifications(rows)
    if validation_errors:
        print("Найдены ошибки в classification:")
        for error in validation_errors:
            print(f"  - {error}")
        print("Допустимые значения:")
        for value in sorted(ALLOWED_CLASSIFICATIONS):
            print(f"  - {value}")
        return 1

    os_counter, class_counter = build_counters(rows)

    print(f"Прочитано записей: {len(rows)}")
    print(format_counter("По ОС:", os_counter))
    print(format_counter("По classification:", class_counter))

    if args.write_report:
        report_text = generate_markdown_report(len(rows), os_counter, class_counter)
        REPORT_PATH.parent.mkdir(parents=True, exist_ok=True)
        REPORT_PATH.write_text(report_text, encoding="utf-8")
        print(f"Markdown-отчёт сохранён: {REPORT_PATH}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
