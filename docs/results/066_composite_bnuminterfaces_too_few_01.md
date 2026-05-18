# Результат эксперимента: 066_composite_bnuminterfaces_too_few_01

##
1. Цель кейса
Проверить реакцию Linux USB-стека на безопасный malformed composite CDC+MSC сценарий, где в configuration descriptor поле `bNumInterfaces` занижено относительно фактического числа interface descriptor, и зафиксировать результат enumeration/driver bind без небезопасного поведения.

##
2. Входные артефакты
- Конфигурация кейса:
- `experiments/cases/066_composite_bnuminterfaces_too_few.json`
- Wrapper log:
- `logs/runs/20260518_210516_066_composite_bnuminterfaces_too_few_01.log`
- Quick analysis:
- `logs/runs/20260518_210516_066_composite_bnuminterfaces_too_few_01.quick_analysis.txt`
- Raw Linux results:
- `results/raw/linux/066_composite_bnuminterfaces_too_few_01_20260518_210544`
- `results/raw/linux/066_composite_bnuminterfaces_too_few_01_20260518_210545`

##
3. Наблюдаемый результат runtime enumeration
- В wrapper-логе: `Runtime USB enumeration status: normal`.
- В raw-статусе: `runtime_enum_status=normal` (`...210545/runtime_status.txt`).
- По `lsusb_tree.txt` устройство привязано по трём интерфейсам:
- `If 0` -> `Driver=cdc_acm`;
- `If 1` -> `Driver=cdc_acm`;
- `If 2` -> `Driver=usb-storage`.

Итог: runtime enumeration успешна, драйверы CDC ACM и USB Mass Storage привязались.

##
4. Linux evidence (wrapper + quick analysis + raw)
- Wrapper (`...210516...log`):
- запуск с `--no-serial-test --allow-runtime-enum-failure`;
- зафиксировано `Runtime USB enumeration status: normal`;
- snapshot-каталоги: `...210544` (основной) и `...210545` (fallback runtime logs).
- Quick analysis (`...210516...quick_analysis.txt`):
- подтверждает `Runtime USB enumeration status: normal`;
- фиксирует наличие runtime snapshot-каталогов и USB-артефактов;
- для целевого окна содержит kernel-сообщения о несогласованности конфигурационного дескриптора.
- Raw (`...210544`, `...210545`):
- `lsusb_v_2e8a_4005.txt`: `bNumInterfaces 2`, при этом перечислены интерфейсы `bInterfaceNumber 0` и `1`;
- `lsusb_tree.txt` (`...210545`): фактически видны `If 0`, `If 1`, `If 2` с bind на `cdc_acm/cdc_acm/usb-storage`;
- `journalctl_k_tail_300.txt` (`...210545`, около `21:05:36`):
  - `config 1 has an invalid interface number: 2 but max is 1`;
  - `config 1 has 3 interfaces, different from the descriptor's value: 2`;
  - затем `New USB device found, idVendor=2e8a, idProduct=4005` и bind `ttyACM0` + `usb-storage 1-4:1.2` (около `21:05:44`).

##
5. Классификация
`KERNEL_WARNING`

Обоснование:
- Linux принял устройство и завершил enumeration/bind;
- одновременно ядро явно зафиксировало предупреждающие сообщения о неконсистентном descriptor (`bNumInterfaces` меньше фактического числа интерфейсов);
- признаков `DRIVER_CRASH` или `SYSTEM_CRASH` в доступных логах нет.

##
6. Уровень уверенности
Высокий.

Причина: несогласованность дескриптора и реакция ядра подтверждаются сразу тремя источниками (wrapper, quick analysis, raw-журналы), включая точные строки kernel-лога и успешный runtime bind.

##
7. Пробелы в доказательной базе
- `journalctl_k_tail*` содержит события нескольких прогонов; привязка к run `21:05` выполнена по временному окну и совпадающим ID устройства.
- Нет usbmon/pcap-трассы control-transfer (`GET_DESCRIPTOR`/`SET_CONFIGURATION`) для покадровой проверки полного дескрипторного обмена.
- Отсутствует отдельный автоматизированный маркер в raw-артефактах, который однозначно связывает каждую строку `journalctl` с конкретным RUN_NAME без временной корреляции.

##
8. Следующие команды верификации
1. `rg -n "Runtime USB enumeration status|Writing snapshot|allow-runtime-enum-failure|no-serial-test" logs/runs/20260518_210516_066_composite_bnuminterfaces_too_few_01.log`
2. `cat results/raw/linux/066_composite_bnuminterfaces_too_few_01_20260518_210545/runtime_status.txt`
3. `rg -n "bNumInterfaces|bInterfaceNumber|bInterfaceClass" results/raw/linux/066_composite_bnuminterfaces_too_few_01_20260518_210544/lsusb_v_2e8a_4005.txt`
4. `rg -n "21:05:36|21:05:44|invalid interface number|different from the descriptor's value|idVendor=2e8a, idProduct=4005|ttyACM0|usb-storage 1-4:1.2" results/raw/linux/066_composite_bnuminterfaces_too_few_01_20260518_210545/journalctl_k_tail_300.txt`
5. `rg -n "Dev 104|If 0|If 1|If 2|Driver=cdc_acm|Driver=usb-storage" results/raw/linux/066_composite_bnuminterfaces_too_few_01_20260518_210545/lsusb_tree.txt`
6. `rg -n "BUG:|Oops|kernel panic|Call Trace|general protection fault|soft lockup|hard lockup" logs/runs/20260518_210516_066_composite_bnuminterfaces_too_few_01.quick_analysis.txt results/raw/linux/066_composite_bnuminterfaces_too_few_01_20260518_210544/journalctl_k_tail_300.txt results/raw/linux/066_composite_bnuminterfaces_too_few_01_20260518_210545/journalctl_k_tail_300.txt`
