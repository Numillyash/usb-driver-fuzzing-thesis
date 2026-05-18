# Результат эксперимента: 062_composite_duplicate_interface_number_01

##
1. Цель кейса
Проверить реакцию Linux USB-стека на malformed composite CDC+MSC дескриптор с дублированием `bInterfaceNumber` и зафиксировать итог runtime enumeration/bind без выполнения небезопасных действий.

##
2. Входные артефакты
- Конфигурация кейса: `experiments/cases/062_composite_duplicate_interface_number.json`
- Wrapper log:

- `logs/runs/20260518_201805_062_composite_duplicate_interface_number_01.log`
- Quick analysis:

- `logs/runs/20260518_201805_062_composite_duplicate_interface_number_01.quick_analysis.txt`
- Raw Linux results:

- `results/raw/linux/062_composite_duplicate_interface_number_01_20260518_201834`

##
3. Наблюдаемый результат runtime enumeration
- В wrapper-логе: `Runtime USB enumeration status: normal`.
- В raw-артефакте `runtime_status.txt`: `runtime_enum_status=normal`.
- Одновременно в kernel-логах этого же прогона зафиксировано:

- `config 1 has 2 interfaces, different from the descriptor's value: 3`;
- `Duplicate descriptor for config 1 interface 1 altsetting 0, skipping`;
- `can't set config #1, error -32`.

Итог: устройство обнаруживается и его дескрипторы читаются, но установка конфигурации завершается ошибкой; полноценный bind composite-функций не подтверждён.

##
4. Linux evidence (wrapper + quick analysis + raw)
- Wrapper (`...201805...log`):

- запуск с `--no-serial-test --allow-runtime-enum-failure`;
- зафиксирован статус `normal`;
- snapshot записан в `...062_composite_duplicate_interface_number_01_20260518_201834`.
- Quick analysis (`...201805...quick_analysis.txt`):

- подтверждает `Runtime USB enumeration status: normal`;
- включает строки из `journalctl` с `Duplicate descriptor ...` и `can't set config #1, error -32` для этого snapshot.
- Raw (`...201834`):

- `runtime_status.txt`: `runtime_enum_status=normal`;
- `lsusb_v_2e8a_4005.txt`: присутствуют два интерфейса с одинаковым `bInterfaceNumber 1` (CDC Data и Mass Storage);
- `journalctl_k_tail.txt`/`journalctl_k_tail_300.txt` (окно `20:18:25-20:18:33`):

  - `config 1 has 2 interfaces, different from the descriptor's value: 3`;
  - `Duplicate descriptor for config 1 interface 1 altsetting 0, skipping`;
  - `can't set config #1, error -32`;
- `tty_devices.txt`: отсутствуют `/dev/ttyACM*` и `/dev/ttyUSB*`, что согласуется с отсутствием подтверждённого bind CDC ACM в итоговом состоянии.

##
5. Классификация
`PARTIAL_ENUM`

Обоснование:
- хост видит устройство и считывает дескрипторы (`New USB device found`, `runtime_enum_status=normal`), поэтому это не `SYSTEM_CRASH` и не `DRIVER_CRASH`;
- установка конфигурации завершается ошибкой `can't set config #1, error -32`, значит перечисление не дошло до устойчивого рабочего состояния функций composite;
- поведение соответствует частичному перечислению с отклонением некорректной конфигурации, а не успешному `OK`.

##
6. Уровень уверенности
Высокий.

Причина: ключевые признаки (duplicate interface descriptor + `can't set config`) подтверждены напрямую в raw kernel-логах и коррелируют с выбранным snapshot и wrapper-запуском.

##
7. Пробелы в доказательной базе
- `journalctl_k_tail*` агрегирует события нескольких запусков, поэтому для атрибуции использованы только строки целевого временного окна (`20:18:25-20:18:33`).
- Отсутствует usbmon/pcap-трейс для пакетного подтверждения стадии, на которой хост возвращает `-32`.
- `dmesg_tail.txt` в snapshot пустой, опора в основном на `journalctl`.

##
8. Следующие команды верификации
1. `rg -n "Runtime USB enumeration status|Writing snapshot|--allow-runtime-enum-failure|--no-serial-test" logs/runs/20260518_201805_062_composite_duplicate_interface_number_01.log`
2. `rg -n "runtime_enum_status" results/raw/linux/062_composite_duplicate_interface_number_01_20260518_201834/runtime_status.txt`
3. `rg -n "bNumInterfaces|bInterfaceNumber|bInterfaceClass" results/raw/linux/062_composite_duplicate_interface_number_01_20260518_201834/lsusb_v_2e8a_4005.txt`
4. `rg -n "20:18:25|20:18:33|config 1 has|Duplicate descriptor|can't set config" results/raw/linux/062_composite_duplicate_interface_number_01_20260518_201834/{journalctl_k_tail.txt,journalctl_k_tail_300.txt}`
5. `rg -n "ttyACM|ttyUSB|usb 1-4" results/raw/linux/062_composite_duplicate_interface_number_01_20260518_201834/{tty_devices.txt,lsusb_tree.txt,journalctl_k_tail.txt}`
6. `rg -n "BUG:|Oops|kernel panic|Call Trace|general protection fault|soft lockup|hard lockup" logs/runs/20260518_201805_062_composite_duplicate_interface_number_01.quick_analysis.txt results/raw/linux/062_composite_duplicate_interface_number_01_20260518_201834/{journalctl_k_tail.txt,journalctl_k_tail_300.txt}`
