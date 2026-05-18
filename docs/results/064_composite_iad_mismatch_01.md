# Результат эксперимента: 064_composite_iad_mismatch_01

##
1. Цель кейса
Проверить реакцию Linux USB-стека на безопасный malformed composite CDC+MSC дескриптор с неконсистентным `Interface Association Descriptor` (IAD), где `bFirstInterface` не совпадает с фактическим началом CDC-функции, и зафиксировать результат enumeration/driver bind без небезопасного поведения.

##
2. Входные артефакты
- Конфигурация кейса:

- `experiments/cases/064_composite_iad_mismatch.json`
- Wrapper log:

- `logs/runs/20260518_204518_064_composite_iad_mismatch_01.log`
- Quick analysis:

- `logs/runs/20260518_204518_064_composite_iad_mismatch_01.quick_analysis.txt`
- Raw Linux results:

- `results/raw/linux/064_composite_iad_mismatch_01_20260518_204546`
- `results/raw/linux/064_composite_iad_mismatch_01_20260518_204547`

##
3. Наблюдаемый результат runtime enumeration
- В wrapper-логе: `Runtime USB enumeration status: normal`.
- В fallback raw-артефакте: `runtime_enum_status=normal` (`...204547/runtime_status.txt`).
- В `lsusb_tree.txt` для целевого запуска устройство привязано по всем трём интерфейсам:

- `If 0` + `If 1` -> `Driver=cdc_acm`;
- `If 2` -> `Driver=usb-storage`.

Итог: несмотря на malformed IAD, Linux успешно выполняет runtime enumeration и bind CDC/MSC без признаков аварийного поведения.

##
4. Linux evidence (wrapper + quick analysis + raw)
- Wrapper (`...204518...log`):

- запуск выполнен с `--no-serial-test --allow-runtime-enum-failure`;
- зафиксировано `Runtime USB enumeration status: normal`;
- сформированы snapshot-каталоги `...204546` и `...204547`.
- Quick analysis (`...204518...quick_analysis.txt`):

- подтверждает `Runtime USB enumeration status: normal`;
- фиксирует наличие IAD в дескрипторе и `bFirstInterface         1`.
- Raw (`...204546` + `...204547`):

- `lsusb_v_2e8a_4005.txt`: `bNumInterfaces=3`, IAD `bFirstInterface=1`, при этом CDC-интерфейсы имеют номера `0` и `1` (неконсистентность подтверждена);
- `journalctl_k_tail_300.txt` (окно целевого запуска около `20:45:38-20:45:46`):

  - `New USB device found, idVendor=2e8a, idProduct=4005`;
  - `cdc_acm 1-4:1.0: ttyACM0: USB ACM device`;
  - `usb-storage 1-4:1.2: USB Mass Storage device detected`;
- `lsusb_tree.txt` (`...204547`): `Driver=cdc_acm` и `Driver=usb-storage` присутствуют одновременно.

##
5. Классификация
`OK`

Обоснование:
- malformed IAD подтверждён по дескрипторам, но Linux перечисляет устройство и выполняет bind CDC+MSC;
- отсутствуют признаки `DRIVER_CRASH`/`SYSTEM_CRASH` (нет `BUG/Oops/panic` в рассмотренных артефактах);
- признаков `PARTIAL_ENUM` или `DRIVER_BIND_ERROR` для целевого окна `20:45` не обнаружено.

##
6. Уровень уверенности
Средний.

Причина: ключевые признаки (`runtime_enum_status=normal`, bind в `lsusb_tree`, события `cdc_acm` и `usb-storage`) согласованы, но `journalctl_k_tail*` содержит события нескольких запусков, поэтому использовалась атрибуция по временному окну целевого прогона.

##
7. Пробелы в доказательной базе
- `journalctl_k_tail*` агрегирует сообщения от предыдущих прогонов; часть строк (`can't set config #1, error -32`, старые `I/O error`) относится к более ранним временным окнам.
- Отсутствует usbmon/pcap-трейс control transfers для покадрового подтверждения обработки IAD на этапе `SET_CONFIGURATION`.
- В `...204547/dmesg_tail*.txt` данные отсутствуют (доступ ограничен), поэтому вывод основан на `journalctl`/`lsusb*`.

##
8. Следующие команды верификации
1. `rg -n "Runtime USB enumeration status|Writing snapshot|--allow-runtime-enum-failure|--no-serial-test" logs/runs/20260518_204518_064_composite_iad_mismatch_01.log`
2. `rg -n "runtime_enum_status" results/raw/linux/064_composite_iad_mismatch_01_20260518_204547/runtime_status.txt`
3. `rg -n "Interface Association|bFirstInterface|bInterfaceNumber|bInterfaceClass" results/raw/linux/064_composite_iad_mismatch_01_20260518_204546/lsusb_v_2e8a_4005.txt`
4. `rg -n "20:45:38|20:45:46|idVendor=2e8a, idProduct=4005|ttyACM0|usb-storage 1-4:1.2" results/raw/linux/064_composite_iad_mismatch_01_20260518_204547/journalctl_k_tail_300.txt`
5. `rg -n "Dev 100|Class=Communications|Class=CDC Data|Class=Mass Storage|Driver=cdc_acm|Driver=usb-storage" results/raw/linux/064_composite_iad_mismatch_01_20260518_204547/lsusb_tree.txt`
6. `rg -n "BUG:|Oops|kernel panic|Call Trace|general protection fault|soft lockup|hard lockup" logs/runs/20260518_204518_064_composite_iad_mismatch_01.quick_analysis.txt results/raw/linux/064_composite_iad_mismatch_01_20260518_204547/{journalctl_k_tail.txt,journalctl_k_tail_300.txt}`
