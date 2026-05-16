# Результат эксперимента: 040_string_invalid_length_01

## 1. Цель кейса
Проверить реакцию Linux USB-стека на неконсистентный string descriptor, где `bLength` в заголовке не соответствует фактической длине строки, и определить фактический исход runtime-перечисления: штатная инициализация, частичное перечисление, ошибка привязки драйвера или отказ.

## 2. Входные артефакты
- Case config: `experiments/cases/040_string_invalid_length.json`
- Wrapper log:
  - `logs/runs/20260516_181739_040_string_invalid_length_01.log`
- Quick analysis:
  - `logs/runs/20260516_181739_040_string_invalid_length_01.quick_analysis.txt`
- Raw Linux results:
  - `results/raw/linux/040_string_invalid_length_01_20260516_181809`
  - `results/raw/linux/040_string_invalid_length_01_20260516_181810`

## 3. Наблюдаемый результат runtime enumeration
- В wrapper-логе зафиксировано: `Runtime USB enumeration status: normal`.
- В raw-артефакте подтверждено: `runtime_enum_status=normal` (`runtime_status.txt`).
- Runtime-устройство присутствует как `2e8a:4005`, создано устройство `/dev/ttyACM0`.

Итог наблюдения: runtime-перечисление прошло штатно.

## 4. Linux evidence (wrapper + quick analysis + raw)
- Wrapper (`...181739...log`): успешный цикл BOOTSEL -> flash -> runtime; статус runtime `normal`; виден `/dev/ttyACM0`.
- Quick analysis (`...181739...quick_analysis.txt`): фиксирует `Runtime USB enumeration status: normal`, указывает snapshot-директории `...181809` и `...181810`.
- Raw (`...181810`):
  - `runtime_status.txt`: `runtime_enum_status=normal`.
  - `lsusb.txt`: присутствует `Bus 001 Device 058: ID 2e8a:4005`.
  - `tty_devices.txt`: присутствует `/dev/ttyACM0`.
  - `lsusb_tree.txt`: интерфейсы `Communications` и `CDC Data` привязаны к `cdc_acm`.
- Raw (`...181809`):
  - `system_snapshot.txt`: устройство `2e8a:4005` обнаружено.
  - `lsusb_v_2e8a_4005.txt`: устройство читается как CDC ACM, строковые дескрипторы возвращаются (например, `iManufacturer`, `iProduct`, `iSerial`).
  - `journalctl_k_tail_300.txt`: есть события подключения `idVendor=2e8a, idProduct=4005`; явных признаков `panic/oops/BUG` для данного прогона не обнаружено.

## 5. Классификация
- Основная: `OK`.

Обоснование:
- runtime enumeration завершился как `normal`;
- драйвер `cdc_acm` успешно привязан, `ttyACM0` создан;
- в доступных логах нет подтверждений `DRIVER_CRASH` или `SYSTEM_CRASH`.

## 6. Уровень уверенности
Средний.

Причина: исход перечисления подтверждается несколькими независимыми артефактами, но из сохраненных логов нельзя строго доказать, что в момент snapshot хост действительно получил именно неконсистентный `bLength` string descriptor (без отдельного USB control-transfer/pcap-трейса).

## 7. Пробелы в доказательной базе
- Нет USB pcap/usbmon-трейса control-transfer, подтверждающего фактическое значение `bLength` строкового дескриптора на шине в этом прогоне.
- `journalctl_k_tail*` агрегирует события нескольких запусков; для более строгой атрибуции полезен узкий временной фильтр вокруг `2026-05-16 18:17:39+03:00`.
- Fallback `dmesg` в wrapper недоступен по правам (`Операция не позволена`), поэтому часть kernel-контекста через этот канал отсутствует.

## 8. Следующие команды верификации
1. `rg -n "Runtime USB enumeration status|runtime_enum_status" logs/runs/20260516_181739_040_string_invalid_length_01.log results/raw/linux/040_string_invalid_length_01_20260516_181810/runtime_status.txt`
2. `rg -n "2e8a:4005|ttyACM0|cdc_acm" results/raw/linux/040_string_invalid_length_01_20260516_181810/{lsusb.txt,tty_devices.txt,lsusb_tree.txt} results/raw/linux/040_string_invalid_length_01_20260516_181809/{system_snapshot.txt,journalctl_k_tail_300.txt}`
3. `rg -n "iManufacturer|iProduct|iSerial|Device Descriptor|String" results/raw/linux/040_string_invalid_length_01_20260516_181809/lsusb_v_2e8a_4005.txt`
4. `rg -n "panic|oops|BUG:|Call Trace|segfault|kernel panic" results/raw/linux/040_string_invalid_length_01_20260516_181809/{journalctl_k_tail_300.txt,dmesg_tail_300.txt,dmesg_full.txt} results/raw/linux/040_string_invalid_length_01_20260516_181810/{journalctl_k_tail.txt,dmesg_tail.txt}`
