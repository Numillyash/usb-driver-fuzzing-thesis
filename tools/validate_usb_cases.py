#!/usr/bin/env python3
"""Валидация JSON-кейсов USB-экспериментов."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path
from typing import Any

REQUIRED_FIELDS = (
    "case_id",
    "name",
    "group",
    "base_persona",
    "description",
    "mutation",
    "expected_windows",
    "expected_linux",
    "risk_level",
    "notes",
)

ALLOWED_RISK_LEVELS = {"safe", "low", "medium"}

BANNED_PATTERNS = (
    "reverse shell",
    "powershell payload",
    "net user",
    "credential theft",
    "persistence",
    "keylogger",
)


def collect_strings(value: Any) -> list[str]:
    """Собирает все строковые значения из произвольной JSON-структуры."""
    if isinstance(value, str):
        return [value]
    if isinstance(value, list):
        result: list[str] = []
        for item in value:
            result.extend(collect_strings(item))
        return result
    if isinstance(value, dict):
        result = []
        for item in value.values():
            result.extend(collect_strings(item))
        return result
    return []


def validate_case_file(path: Path) -> list[str]:
    errors: list[str] = []

    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        return [f"Некорректный JSON: {exc}"]
    except OSError as exc:
        return [f"Ошибка чтения файла: {exc}"]

    if not isinstance(data, dict):
        return ["Корневой JSON-объект должен быть объектом (dictionary)."]

    for field in REQUIRED_FIELDS:
        if field not in data:
            errors.append(f"Отсутствует обязательное поле: '{field}'")

    filename_prefix = path.stem.split("_", 1)[0]
    case_id = data.get("case_id")
    if case_id is None:
        pass
    elif not isinstance(case_id, str):
        errors.append("Поле 'case_id' должно быть строкой.")
    elif case_id != filename_prefix:
        errors.append(
            "Поле 'case_id' не совпадает с префиксом имени файла: "
            f"ожидалось '{filename_prefix}', получено '{case_id}'"
        )

    risk_level = data.get("risk_level")
    if risk_level is None:
        pass
    elif not isinstance(risk_level, str):
        errors.append("Поле 'risk_level' должно быть строкой.")
    elif risk_level not in ALLOWED_RISK_LEVELS:
        allowed = ", ".join(sorted(ALLOWED_RISK_LEVELS))
        errors.append(
            f"Недопустимое значение 'risk_level': '{risk_level}'. Допустимо: {allowed}"
        )

    lowered_patterns = [(p, re.compile(re.escape(p), re.IGNORECASE)) for p in BANNED_PATTERNS]
    for text in collect_strings(data):
        for phrase, pattern in lowered_patterns:
            if pattern.search(text):
                errors.append(
                    "Обнаружен запрещенный контент: "
                    f"фраза '{phrase}' присутствует в строковом поле JSON"
                )

    return errors


def main() -> int:
    repo_root = Path(__file__).resolve().parent.parent
    cases_dir = repo_root / "experiments" / "cases"

    if not cases_dir.is_dir():
        print(f"ERROR: каталог не найден: {cases_dir}")
        return 2

    case_files = sorted(cases_dir.rglob("*.json"))
    total = len(case_files)
    passed = 0
    failed = 0

    if total == 0:
        print(f"WARNING: в {cases_dir} не найдено JSON-файлов для проверки.")

    for case_file in case_files:
        errors = validate_case_file(case_file)
        rel_path = case_file.relative_to(repo_root)
        if errors:
            failed += 1
            print(f"FAIL: {rel_path}")
            for err in errors:
                print(f"  - {err}")
        else:
            passed += 1
            print(f"PASS: {rel_path}")

    print("\nSummary:")
    print(f"  total cases: {total}")
    print(f"  passed cases: {passed}")
    print(f"  failed cases: {failed}")

    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
