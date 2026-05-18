# Результат эксперимента: 065_composite_bnuminterfaces_too_many_01

##
1. Цель кейса
Проверить реакцию Linux USB-стека на безопасный malformed composite CDC+MSC сценарий с завышенным `bNumInterfaces` относительно фактического числа interface descriptor, зафиксировав результат enumeration/driver bind без небезопасного поведения.

##
2. Входные артефакты
- Конфигурация кейса:

- `experiments/cases/065_composite_bnuminterfaces_too_many.json`
- Wrapper log:

- `logs/runs/20260518_205701_065_composite_bnuminterfaces_too_many_01.log`
- Quick analysis:

- `logs/runs/20260518_205701_065_composite_bnuminterfaces_too_many_01.quick_analysis.txt`
- Raw Linux results:

- `results/raw/linux/065_composite_bnuminterfaces_too_many_01_20260518_205730`

##
3. Наблюдаемый результат runtime enumeration
- В wrapper-логе: `Runtime USB enumeration status: normal`.
- В raw-статусе: `runtime_enum_status=normal` (`results/raw/linux/065_composite_bnuminterfaces_too_many_01_20260518_205730/runtime_status.txt`).
- В `lsusb_tree.txt` устройство привязано по трём интерфейсам:

- `If 0` + `If 1` -> `Driver=cdc_acm`;
- `If 2` -> `Driver=usb-storage`.

Итог: runtime enumeration и bind CDC/MSC прошли успешно.

##
4. Linux evidence (wrapper + quick analysis + raw)
- Wrapper (`...205701...log`):

- запуск с `--no-serial-test --allow-runtime-enum-failure`;
- зафиксировано `Runtime USB enumeration status: normal`;
- snapshot записан в `.../065_composite_bnuminterfaces_too_many_01_20260518_205730`.
- Quick analysis (`...205701...quick_analysis.txt`):

- подтверждает `Runtime USB enumeration status: normal`;
- подтверждает наличие целевого runtime snapshot-каталога;
- фиксирует `lsusb_tree` с `cdc_acm` и `usb-storage`.
- Raw (`...205730`):

- `runtime_status.txt`: `runtime_enum_status=normal`;
- `lsusb_tree.txt`: `If 0/1/2` с драйверами `cdc_acm/cdc_acm/usb-storage`;
- `journalctl_k_tail_300.txt` (окно целевого прогона около `20:57:21-20:57:29`):
  - `New USB device found, idVendor=2e8a, idProduct=4005`;
  - `cdc_acm 1-4:1.0: ttyACM0: USB ACM device`;
  - `usb-storage 1-4:1.2: USB Mass Storage device detected`.

Дополнительно по дескриптору целевого устройства (`lsusb_v_2e8a_4005.txt`):
- `bNumInterfaces          3`;
- перечислены `bInterfaceNumber 0`, `1`, `2`.

Это не подтверждает ожидаемую неконсистентность «`bNumInterfaces` больше фактического числа интерфейсов» в захваченном runtime-дескрипторе.

##
5. Классификация
`OK`

Обоснование:
- Linux успешно перечислил устройство и выполнил bind CDC+MSC;
- признаков `PARTIAL_ENUM`/`DRIVER_BIND_ERROR` для целевого временного окна нет;
- в доступных логах нет признаков `DRIVER_CRASH`/`SYSTEM_CRASH` (`BUG/Oops/panic` не зафиксированы).

##
6. Уровень уверенности
Средний.

Причина: ключевые признаки успешной enumeration согласованы между wrapper/quick-analysis/raw, но журнал `journalctl_k_tail*` содержит события нескольких запусков, и атрибуция выполнялась по временному окну целевого прогона.

##
7. Пробелы в доказательной базе
- В захваченном `lsusb_v_2e8a_4005.txt` не видно целевой мутации `bNumInterfaces` (наблюдается согласованная тройка интерфейсов 0/1/2). Нужна отдельная проверка, что именно прошивка для кейса `065` действительно экспонировала ожидаемую malformed-конфигурацию.
- `journalctl_k_tail*` агрегирует сообщения нескольких прогонов; часть строк не относится строго к run `20:57`.
- Нет usbmon/pcap control-transfer трассы для покадровой проверки фактического configuration descriptor в момент `GET_DESCRIPTOR/SET_CONFIGURATION`.

##
8. Следующие команды верификации
1. `rg -n "Runtime USB enumeration status|Writing snapshot|allow-runtime-enum-failure|no-serial-test" logs/runs/20260518_205701_065_composite_bnuminterfaces_too_many_01.log`
2. `cat results/raw/linux/065_composite_bnuminterfaces_too_many_01_20260518_205730/runtime_status.txt`
3. `rg -n "bNumInterfaces|bInterfaceNumber|bInterfaceClass" results/raw/linux/065_composite_bnuminterfaces_too_many_01_20260518_205730/lsusb_v_2e8a_4005.txt`
4. `rg -n "20:57:21|20:57:29|idVendor=2e8a, idProduct=4005|ttyACM0|usb-storage 1-4:1.2" results/raw/linux/065_composite_bnuminterfaces_too_many_01_20260518_205730/journalctl_k_tail_300.txt`
5. `rg -n "Dev 102|Class=Communications|Class=CDC Data|Class=Mass Storage|Driver=cdc_acm|Driver=usb-storage" results/raw/linux/065_composite_bnuminterfaces_too_many_01_20260518_205730/lsusb_tree.txt`
6. `rg -n "BUG:|Oops|kernel panic|Call Trace|general protection fault|soft lockup|hard lockup" logs/runs/20260518_205701_065_composite_bnuminterfaces_too_many_01.quick_analysis.txt results/raw/linux/065_composite_bnuminterfaces_too_many_01_20260518_205730/{journalctl_k_tail.txt,journalctl_k_tail_300.txt}`
