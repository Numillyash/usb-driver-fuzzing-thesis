# Результаты экспериментов

Каталог предназначен для хранения результатов выполнения кейсов из `experiments/cases/`.

Рекомендуемая организация:
- подкаталог по ОС и версии;
- подкаталог по дате серии экспериментов;
- отдельные файлы отчётов по шаблону `experiments/templates/result_template.md`;
- вложения с логами и USB-трейсами.

Минимальный состав артефактов по кейсу:
- заполненный отчёт;
- системные логи ОС;
- USB-трейс перечисления;
- краткая классификация реакции.

## Сводный CSV для серии экспериментов

Для агрегирования результатов используйте файл:
- `experiments/results/summary.csv`

Если файла ещё нет, создайте его из шаблона:

```bash
cp experiments/results/summary_template.csv experiments/results/summary.csv
```

Столбцы `summary.csv`:
- `case_id`
- `case_name`
- `group`
- `base_persona`
- `mutation_summary`
- `os`
- `enumeration_result`
- `driver_result`
- `log_artifacts`
- `trace_artifacts`
- `stability_impact`
- `classification`
- `notes`

Допустимые значения `classification`:
- `normal_handling`
- `enumeration_rejected`
- `descriptor_parse_error`
- `driver_error`
- `device_side_failure`
- `host_side_instability`
- `inconclusive`

## Генерация сводки

Скрипт `tools/summarize_usb_results.py`:
- читает `experiments/results/summary.csv` (если файл существует);
- проверяет корректность `classification`;
- печатает количество записей по ОС и по `classification`;
- опционально сохраняет отчёт `experiments/results/summary_report.md`.

Примеры запуска:

```bash
python3 tools/summarize_usb_results.py
```

```bash
python3 tools/summarize_usb_results.py --write-report
```
