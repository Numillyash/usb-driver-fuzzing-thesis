# Результат эксперимента: 010_device_blength_too_short_01

## Идентификаторы запуска
- Wrapper log: `logs/runs/20260516_041440_010_device_blength_too_short_01.log`
- Quick analysis: `logs/runs/20260516_041440_010_device_blength_too_short_01.quick_analysis.txt`
- Raw results:
  - `results/raw/linux/010_device_blength_too_short_01_20260516_041508`
  - `results/raw/linux/010_device_blength_too_short_01_20260516_041509`
- Платформа хоста: Linux `6.17.0-19-generic` (`system_snapshot.txt`, `runtime_status.txt`)

## Цель и гипотеза
Проверить, как Linux USB-стек обрабатывает кейс с укороченным `bLength` в device descriptor и приведёт ли это к корректному отклонению/частичной инициализации без критической дестабилизации системы.

## Наблюдения (фактические признаки)
1. В wrapper зафиксирован неполный runtime enum:
- `Runtime USB enumeration status: partial` (`logs/runs/20260516_041440_010_device_blength_too_short_01.log:98`).

2. В runtime status также зафиксирован partial:
- `runtime_enum_status=partial` (`results/raw/linux/010_device_blength_too_short_01_20260516_041509/runtime_status.txt:3`).

3. В kernel logs есть признаки проблем дескриптора/конфигурирования:
- `unable to get BOS descriptor or descriptor too short` (`...041508/journalctl_k_tail_300.txt:53`, `:88`).
- `can't set config #1, error -32` (`...041508/journalctl_k_tail_300.txt:60`).

4. Одновременно видны признаки частичной/периодической инициализации устройства в других интервалах хвоста лога (исторические записи в tail):
- `New USB device found, idVendor=2e8a, idProduct=4005` (`...041508/journalctl_k_tail_300.txt:295`).
- `cdc_acm ... ttyACM0: USB ACM device` (например, `...041508/journalctl_k_tail_300.txt:189`).

5. В fallback-снимке tty-узлы для текущей точки наблюдения отсутствуют:
- `ls: невозможно получить доступ к '/dev/ttyACM*'` (`...041509/tty_devices.txt:1`).

6. Недостаток сбора `dmesg` через fallback:
- `dmesg: ... Операция не позволена` (`logs/runs/20260516_041440_010_device_blength_too_short_01.log:168`).

## Классификация результата
- Основная классификация: `PARTIAL_ENUM`.
- Сопутствующая классификация по журналу ядра: `KERNEL_WARNING`.

Обоснование: есть явный `partial` в wrapper/runtime status и kernel-сообщения о проблемах дескриптора/конфигурации, при этом нет подтверждений `DRIVER_CRASH` или `SYSTEM_CRASH`.

## Интерпретация
Результат согласуется с целевым безопасным сценарием robustness-тестирования: ОС не демонстрирует подтверждённый критический сбой, но фиксируются ошибки обработки дескрипторных данных и неполная инициализация.

## Ограничения доказательной базы
- `journalctl_k_tail_300.txt` и `journalctl_k_tail.txt` содержат исторический хвост, включающий события до времени конкретного прогона; для более строгой атрибуции нужен временной фильтр/отметка окна.
- Снимок `dmesg` в fallback-этапе неполный из-за прав доступа.
- Текущий отчёт не является доказательством kernel bug/crash.

## Рекомендованные следующие шаги (в рамках стенда)
1. Добавить в скрипт сбора жёсткое окно времени `start_ts..end_ts` для `journalctl -k`.
2. Явно фиксировать VID:PID и serial именно для текущего запуска в отдельный файл `runtime_device_identity.txt`.
3. На Linux выполнять повтор не менее 3 раз и сравнивать долю `PARTIAL_ENUM` против baseline `000_baseline_cdc`.
