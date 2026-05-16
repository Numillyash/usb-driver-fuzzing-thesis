# Результат эксперимента: 032_endpoint_direction_mismatch_01

## 1. Цель кейса
Проверить реакцию Linux USB-стека на аномалию в endpoint descriptor, где направление endpoint (`bEndpointAddress`) намеренно изменено относительно ожидаемого транспортного профиля интерфейса CDC ACM, и определить фактический исход перечисления: штатная инициализация, частичное перечисление или отказ привязки драйвера.

## 2. Входные артефакты
- Case config: `experiments/cases/032_endpoint_direction_mismatch.json`
- Wrapper log:
  - `logs/runs/20260516_180044_032_endpoint_direction_mismatch_01.log`
- Quick analysis:
  - `logs/runs/20260516_180044_032_endpoint_direction_mismatch_01.quick_analysis.txt`
- Raw Linux results:
  - `results/raw/linux/032_endpoint_direction_mismatch_01_20260516_180115`

## 3. Наблюдаемый результат runtime enumeration
- В wrapper-логе зафиксировано: `Runtime USB enumeration status: normal`.
- В raw-артефакте подтверждено: `runtime_enum_status=normal` (`runtime_status.txt`).
- Runtime-устройство присутствует в `lsusb` как `2e8a:4005`, создано устройство `/dev/ttyACM0`.

Итог наблюдения: runtime-перечисление прошло штатно.

## 4. Linux evidence (wrapper + quick analysis + raw)
- Wrapper (`...180044...log`): успешный BOOTSEL -> flash -> runtime цикл; runtime-статус `normal`; виден `/dev/ttyACM0`.
- Quick analysis (`...180044...quick_analysis.txt`): фиксирует `Runtime USB enumeration status: normal`; связывает прогон со snapshot `032_endpoint_direction_mismatch_01_20260516_180115`.
- Raw (`...180115`):
  - `runtime_status.txt`: `runtime_enum_status=normal`.
  - `lsusb.txt`, `system_snapshot.txt`: присутствует `2e8a:4005`.
  - `tty_devices.txt`: присутствует `/dev/ttyACM0`.
  - `lsusb_tree.txt`: оба интерфейса (Communications и CDC Data) привязаны к `cdc_acm`.
  - `journalctl_k_tail.txt`/`journalctl_k_tail_300.txt`: есть события `cdc_acm ... ttyACM0: USB ACM device`; явных признаков `panic/oops/BUG` не обнаружено.

Дополнительное наблюдение по дескрипторам:
- В `lsusb_v_2e8a_4005.txt` видны endpoint-адреса, соответствующие обычному CDC ACM профилю (`0x81` interrupt IN, `0x02` bulk OUT, `0x82` bulk IN). Явное подтверждение mismatch-направления в этом snapshot отсутствует.

## 5. Классификация
- Основная: `OK`.

Обоснование:
- перечисление в runtime завершено как `normal`;
- драйвер `cdc_acm` привязан, `ttyACM0` создан;
- по доступным логам отсутствуют признаки `DRIVER_BIND_ERROR`, `DRIVER_CRASH` или `SYSTEM_CRASH`.

## 6. Уровень уверенности
Средний.

Причина: исход перечисления подтверждён несколькими независимыми артефактами, но сохранённый `lsusb -v` не демонстрирует ожидаемую аномалию направления endpoint в момент snapshot.

## 7. Пробелы в доказательной базе
- Нет независимого USB control-transfer/pcap-трейса, который подтверждает, что хост действительно получил дескриптор с direction mismatch в этом прогоне.
- `journalctl_k_tail*` содержит события нескольких запусков; для строгой атрибуции полезна более узкая временная выборка строго вокруг `2026-05-16 18:00:44+03:00`.
- Fallback `dmesg` в wrapper не собран по правам (`Операция не позволена`), поэтому часть kernel-контекста недоступна этим каналом.

## 8. Следующие команды верификации
1. `rg -n "Runtime USB enumeration status|runtime_enum_status" logs/runs/20260516_180044_032_endpoint_direction_mismatch_01.log results/raw/linux/032_endpoint_direction_mismatch_01_20260516_180115/runtime_status.txt`
2. `rg -n "2e8a:4005|ttyACM0|cdc_acm" results/raw/linux/032_endpoint_direction_mismatch_01_20260516_180115/{lsusb.txt,system_snapshot.txt,tty_devices.txt,lsusb_tree.txt,journalctl_k_tail.txt,journalctl_k_tail_300.txt}`
3. `rg -n "bEndpointAddress|Endpoint Descriptor|wMaxPacketSize" results/raw/linux/032_endpoint_direction_mismatch_01_20260516_180115/lsusb_v_2e8a_4005.txt`
4. `rg -n "panic|oops|BUG:|segfault|Call Trace|crash" results/raw/linux/032_endpoint_direction_mismatch_01_20260516_180115/{journalctl_k_tail.txt,journalctl_k_tail_300.txt,dmesg_tail.txt,dmesg_tail_300.txt}`
