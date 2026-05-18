# Результат эксперимента: 063_composite_duplicate_endpoint_address_01

##
1. Цель кейса
Проверить реакцию Linux USB-стека на malformed composite CDC+MSC дескриптор с дублированием `bEndpointAddress` (повтор `0x82` для CDC IN и MSC IN) и зафиксировать результат runtime enumeration/bind без небезопасного поведения.

##
2. Входные артефакты
- Конфигурация кейса:

- `experiments/cases/063_composite_duplicate_endpoint_address.json`
- Wrapper log:

- `logs/runs/20260518_202931_063_composite_duplicate_endpoint_address_01.log`
- Quick analysis:

- `logs/runs/20260518_202931_063_composite_duplicate_endpoint_address_01.quick_analysis.txt`
- Raw Linux results:

- `results/raw/linux/063_composite_duplicate_endpoint_address_01_20260518_202959`

##
3. Наблюдаемый результат runtime enumeration
- В wrapper-логе: `Runtime USB enumeration status: normal`.
- В raw-артефакте `runtime_status.txt`: `runtime_enum_status=normal`.
- В kernel-логе для целевого запуска (`20:29:50`) зафиксировано:

- `config 1 interface 2 altsetting 0 has a duplicate endpoint with address 0x82, skipping`;
- после этого появляются `cdc_acm ... ttyACM0` и `usb-storage 1-4:1.2: USB Mass Storage device detected`.

Итог: устройство перечисляется, Linux явно отбрасывает конфликтующий endpoint descriptor и продолжает инициализацию, но наблюдаются признаки неполного/нестабильного bind Mass Storage в снимке состояния.

##
4. Linux evidence (wrapper + quick analysis + raw)
- Wrapper (`...202931...log`):

- запуск с `--no-serial-test --allow-runtime-enum-failure`;
- `Runtime USB enumeration status: normal`;
- snapshot записан в `...063_composite_duplicate_endpoint_address_01_20260518_202959`;
- на шаге runtime-проверки виден `/dev/ttyACM0`, но в `lsblk` на этом шаге нет runtime-диска `sdb`.
- Quick analysis (`...202931...quick_analysis.txt`):

- подтверждает строку `Runtime USB enumeration status: normal`;
- содержит строку kernel про duplicate endpoint `0x82` и последующие `ttyACM0`/`usb-storage` для snapshot `...202959`.
- Raw (`...202959`):

- `runtime_status.txt`: `runtime_enum_status=normal`;
- `lsusb_v_2e8a_4005.txt`: duplicate endpoint подтверждён дескрипторами (`bEndpointAddress 0x82` у CDC Data IN и у MSC IN);
- `journalctl_k_tail.txt`:

  - `20:29:50 ... duplicate endpoint with address 0x82, skipping`;
  - `20:29:58 ... cdc_acm ... ttyACM0`;
  - `20:29:58 ... usb-storage 1-4:1.2: USB Mass Storage device detected`;
- `lsusb_tree.txt`: для `If 2, Class=Mass Storage` отображается `Driver=[none]`, что указывает на проблемный bind в момент съёма дерева устройств.

##
5. Классификация
`DRIVER_BIND_ERROR`

Обоснование:
- это не `DRIVER_CRASH` и не `SYSTEM_CRASH`: нет признаков panic/oops/падения системы;
- enumeration формально проходит (`runtime_enum_status=normal`, устройство видно в `lsusb`);
- malformed endpoint обрабатывается с деградацией (`duplicate endpoint ... skipping`), и для Mass Storage есть несогласованность bind-состояния (`usb-storage detected` в журнале при `Driver=[none]` в `lsusb_tree`), что ближе к ошибке/нестабильности привязки драйвера, а не к полностью успешному `OK`.

##
6. Уровень уверенности
Средний.

Причина: ключевой сигнал duplicate endpoint подтверждён напрямую; однако `journalctl_k_tail*` агрегирует события нескольких запусков, а артефакты bind (journal vs `lsusb_tree`) дают частично расходящуюся картину по моментам времени.

##
7. Пробелы в доказательной базе
- `journalctl_k_tail*` содержит события не только целевого кейса; для атрибуции использованы строки окна `20:29:50-20:29:58`.
- Нет usbmon/pcap-трейса control transfer/SET_CONFIGURATION для точного восстановления последовательности bind.
- `dmesg_tail.txt` и `dmesg_tail_300.txt` пустые, поэтому вывод основан главным образом на `journalctl` и `lsusb*`.

##
8. Следующие команды верификации
1. `rg -n "Runtime USB enumeration status|Writing snapshot|--allow-runtime-enum-failure|--no-serial-test" logs/runs/20260518_202931_063_composite_duplicate_endpoint_address_01.log`
2. `rg -n "runtime_enum_status" results/raw/linux/063_composite_duplicate_endpoint_address_01_20260518_202959/runtime_status.txt`
3. `rg -n "bInterfaceNumber|bInterfaceClass|bEndpointAddress\s+0x82" results/raw/linux/063_composite_duplicate_endpoint_address_01_20260518_202959/lsusb_v_2e8a_4005.txt`
4. `rg -n "20:29:50|20:29:58|duplicate endpoint|ttyACM0|usb-storage 1-4:1.2|can't set config" results/raw/linux/063_composite_duplicate_endpoint_address_01_20260518_202959/{journalctl_k_tail.txt,journalctl_k_tail_300.txt}`
5. `rg -n "Dev 098|Mass Storage|Driver=\[none\]" results/raw/linux/063_composite_duplicate_endpoint_address_01_20260518_202959/lsusb_tree.txt`
6. `rg -n "BUG:|Oops|kernel panic|Call Trace|general protection fault|soft lockup|hard lockup" logs/runs/20260518_202931_063_composite_duplicate_endpoint_address_01.quick_analysis.txt results/raw/linux/063_composite_duplicate_endpoint_address_01_20260518_202959/{journalctl_k_tail.txt,journalctl_k_tail_300.txt}`
