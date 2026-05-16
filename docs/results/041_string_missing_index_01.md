# Результат эксперимента: 041_string_missing_index_01

## 1. Цель кейса
Проверить реакцию Linux USB-стека на случай, когда в дескрипторах устройства указан строковый индекс (`iManufacturer`, `iProduct` или `iSerialNumber`), но соответствующая строка отсутствует или недоступна, и зафиксировать фактический исход runtime-перечисления.

## 2. Входные артефакты
- Case config: `experiments/cases/041_string_missing_index.json`
- Wrapper log:
  - `logs/runs/20260516_182707_041_string_missing_index_01.log`
- Quick analysis:
  - `logs/runs/20260516_182707_041_string_missing_index_01.quick_analysis.txt`
- Raw Linux results:
  - `results/raw/linux/041_string_missing_index_01_20260516_182738`

## 3. Наблюдаемый результат runtime enumeration
- В wrapper-логе зафиксировано: `Runtime USB enumeration status: normal`.
- В raw-артефакте подтверждено: `runtime_enum_status=normal` (`runtime_status.txt`).
- Runtime-устройство присутствует как `2e8a:4005`, создано `/dev/ttyACM0`, драйвер `cdc_acm` привязан к обоим CDC-интерфейсам.

Итог наблюдения: runtime-перечисление прошло штатно.

## 4. Linux evidence (wrapper + quick analysis + raw)
- Wrapper (`...182707...log`):
  - успешный цикл BOOTSEL -> flash -> runtime;
  - `Runtime USB enumeration status: normal`;
  - в runtime-снимке видны `2e8a:4005` и `/dev/ttyACM0`.
- Quick analysis (`...182707...quick_analysis.txt`):
  - подтверждает статус `normal`;
  - ссылается на raw-директорию `results/raw/linux/041_string_missing_index_01_20260516_182738`;
  - фиксирует привязку `cdc_acm` и наличие `ttyACM0`.
- Raw (`...182738`):
  - `runtime_status.txt`: `runtime_enum_status=normal`;
  - `lsusb.txt` и `system_snapshot.txt`: устройство `2e8a:4005` обнаружено;
  - `lsusb_tree.txt`: интерфейсы `Communications` и `CDC Data` обслуживаются `cdc_acm`;
  - `tty_devices.txt`: присутствует `/dev/ttyACM0`;
  - `journalctl_k_tail.txt`: есть событие `cdc_acm ... ttyACM0: USB ACM device`;
  - `lsusb_v_2e8a_4005.txt`: строковые поля читаются (`iManufacturer=1`, `iProduct=2`, `iSerial=3`) в финальном состоянии снимка.

## 5. Классификация
- Основная: `OK`.

Обоснование:
- перечисление завершено как `normal`;
- драйвер `cdc_acm` привязан, `ttyACM0` создан;
- подтверждений `DRIVER_CRASH` или `SYSTEM_CRASH` в предоставленных артефактах нет.

## 6. Уровень уверенности
Средний.

Причина: итог перечисления и привязка драйвера подтверждаются несколькими независимыми артефактами, но в этом наборе нет прямого USB control-transfer/pcap-доказательства, что во время данного прогона хост действительно получил отсутствующий строковый индекс именно в наблюдаемой финальной сессии.

## 7. Пробелы в доказательной базе
- Нет USB pcap/usbmon-трейса setup/data stage для запросов string descriptors.
- `journalctl_k_tail*` содержит события нескольких запусков, поэтому нужна более строгая привязка по узкому временному окну к запуску `2026-05-16T18:27:07+03:00`.
- В wrapper-логе fallback-вызов `dmesg` недоступен по правам (`Операция не позволена`), поэтому часть kernel-контекста отсутствует именно в этом канале.

## 8. Следующие команды верификации
1. `rg -n "Runtime USB enumeration status|runtime_enum_status" logs/runs/20260516_182707_041_string_missing_index_01.log results/raw/linux/041_string_missing_index_01_20260516_182738/runtime_status.txt`
2. `rg -n "2e8a:4005|ttyACM0|cdc_acm" results/raw/linux/041_string_missing_index_01_20260516_182738/{lsusb.txt,system_snapshot.txt,lsusb_tree.txt,tty_devices.txt,journalctl_k_tail.txt}`
3. `rg -n "iManufacturer|iProduct|iSerial" results/raw/linux/041_string_missing_index_01_20260516_182738/lsusb_v_2e8a_4005.txt`
4. `rg -n "panic|oops|BUG:|Call Trace|segfault|kernel panic|WARNING:" logs/runs/20260516_182707_041_string_missing_index_01.log results/raw/linux/041_string_missing_index_01_20260516_182738/{journalctl_k_tail.txt,journalctl_k_tail_300.txt,dmesg_tail_300.txt,dmesg_full.txt}`
