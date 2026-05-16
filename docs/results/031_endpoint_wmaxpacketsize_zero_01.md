# Результат эксперимента: 031_endpoint_wmaxpacketsize_zero_01

## 1. Цель кейса
Проверить реакцию Linux USB-стека на аномальный endpoint descriptor, где для рабочего endpoint задано `wMaxPacketSize=0`, и определить, приводит ли это к отказу перечисления, частичному перечислению, ошибке привязки драйвера или к штатной инициализации.

## 2. Входные артефакты
- Case config: `experiments/cases/031_endpoint_wmaxpacketsize_zero.json`
- Wrapper logs:
  - `logs/runs/20260516_174920_031_endpoint_wmaxpacketsize_zero_01.log`
  - `logs/runs/20260516_175202_031_endpoint_wmaxpacketsize_zero_01.log`
- Quick analysis:
  - `logs/runs/20260516_175202_031_endpoint_wmaxpacketsize_zero_01.quick_analysis.txt`
- Raw Linux results:
  - `results/raw/linux/031_endpoint_wmaxpacketsize_zero_01_20260516_174951`
  - `results/raw/linux/031_endpoint_wmaxpacketsize_zero_01_20260516_175233`

## 3. Наблюдаемый результат runtime enumeration
- В обоих wrapper-логах зафиксировано: `Runtime USB enumeration status: normal`.
- В raw-артефакте второго прогона подтверждено: `runtime_enum_status=normal` (`runtime_status.txt`).
- Runtime-устройство присутствует в `lsusb` как `2e8a:4005`, присутствует устройство `/dev/ttyACM0`.

Итог наблюдения: в зафиксированных прогонах runtime-перечисление прошло штатно.

## 4. Linux evidence (wrapper + quick analysis + raw)
- Wrapper (`...174920...log`, `...175202...log`): успешный цикл BOOTSEL -> flash -> runtime, статус runtime `normal`.
- Quick analysis (`...175202...quick_analysis.txt`): фиксирует `runtime_enum_status=normal` для snapshot `20260516_175233`.
- Raw (`...175233`):
  - `runtime_status.txt`: `runtime_enum_status=normal`.
  - `lsusb.txt`, `system_snapshot.txt`: устройство `2e8a:4005` обнаружено.
  - `tty_devices.txt`: присутствует `/dev/ttyACM0`.
  - `lsusb_tree.txt`: интерфейсы привязаны к `cdc_acm`.
  - `journalctl_k_tail*.txt`: есть события перечисления и создание `ttyACM0`; признаков `panic/oops/BUG` не найдено.

Дополнительное наблюдение по содержимому дескрипторов:
- В `lsusb_v_2e8a_4005.txt` (snapshot `...175233`) для endpoint-ов видны значения `wMaxPacketSize` `0x0008` и `0x0040`, а не ноль.

## 5. Классификация
- Основная: `OK`.

Обоснование:
- runtime enumeration завершился как `normal`;
- привязка `cdc_acm` выполнена, `ttyACM0` создан;
- в предоставленных логах отсутствуют признаки `DRIVER_BIND_ERROR`, `KERNEL_WARNING` с аварийными последствиями, `DRIVER_CRASH` или `SYSTEM_CRASH`.

## 6. Уровень уверенности
Средний.

Причина: факт штатного перечисления подтверждается несколькими независимыми артефактами, однако из сохраненного `lsusb -v` не подтверждается предъявление endpoint с `wMaxPacketSize=0` в момент snapshot.

## 7. Пробелы в доказательной базе
- Нет независимого USB control-transfer/pcap-трейса, подтверждающего, что хост получил именно дескриптор с `wMaxPacketSize=0`.
- `journalctl_k_tail*` содержит события нескольких запусков; для строгой атрибуции нужен более узкий временной фильтр на один прогон.
- Fallback `dmesg` в wrapper недоступен по правам (`Операция не позволена`), поэтому часть kernel-контекста не собрана этим каналом.

## 8. Следующие команды верификации
1. `rg -n "Runtime USB enumeration status|runtime_enum_status" logs/runs/*031_endpoint_wmaxpacketsize_zero_01*.log results/raw/linux/031_endpoint_wmaxpacketsize_zero_01_20260516_175233/runtime_status.txt`
2. `rg -n "2e8a:4005|ttyACM0|cdc_acm" results/raw/linux/031_endpoint_wmaxpacketsize_zero_01_20260516_175233/{lsusb.txt,system_snapshot.txt,lsusb_tree.txt,tty_devices.txt,journalctl_k_tail.txt,journalctl_k_tail_300.txt}`
3. `rg -n "wMaxPacketSize|bEndpointAddress|Endpoint Descriptor" results/raw/linux/031_endpoint_wmaxpacketsize_zero_01_20260516_175233/lsusb_v_2e8a_4005.txt`
4. `rg -n "panic|oops|BUG:|segfault|crash" results/raw/linux/031_endpoint_wmaxpacketsize_zero_01_20260516_175233/{journalctl_k_tail.txt,journalctl_k_tail_300.txt,dmesg_tail.txt,dmesg_tail_300.txt}`
