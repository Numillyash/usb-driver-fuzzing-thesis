# Сводка Linux MSC-кейсов (050-059)

## 1. Назначение MSC Linux robustness-тестирования

Цель блока `050-059` — проверить устойчивость Linux USB/MSC-стека при безопасных аномалиях поведения MSC-устройства на RP2040.

Важное разделение:

- успешность USB enumeration;
- привязка драйвера `usb-storage`;
- состояние SCSI/block/FS-слоя;
- наличие или отсутствие crash-событий.

## 2. Методика

Использовалось безопасное исследовательское устройство на RP2040 в режиме MSC с VID/PID `2e8a:4006`.

Принципы стенда:

- поведение носителя read-only/RAM-backed;
- доступ к реальным дискам хоста не выполнялся;
- запись, форматирование и ручное монтирование тестового носителя оператором не выполнялись;
- источники доказательств: wrapper-логи, quick-analysis, `lsusb`, `lsusb -t`, `journalctl`, `dmesg`, `lsblk`, per-case отчёты.

## 3. Таксономия результатов

Использовалась единая шкала:

- `OK`;
- `EXPECTED_REJECT`;
- `PARTIAL_ENUM`;
- `DRIVER_BIND_ERROR`;
- `USERSPACE_FAILURE`;
- `KERNEL_WARNING`;
- `DRIVER_CRASH`;
- `SYSTEM_CRASH`.

## 4. Таблица результатов 050-059

| Кейс | Краткое описание | Класс |
|---|---|---|
| `050_msc_baseline_01` | baseline MSC initial run | `PARTIAL_ENUM` |
| `050_msc_baseline_repeat_01` | baseline MSC repeat after MSC-aware runner | `OK` |
| `051_msc_zero_block_size_01` | логический размер блока = 0 | `PARTIAL_ENUM` |
| `052_msc_zero_capacity_01` | ёмкость = 0 блоков | `PARTIAL_ENUM` |
| `053_msc_huge_capacity_01` | аномально большая ёмкость | `PARTIAL_ENUM` |
| `054_msc_read_capacity_short_response_01` | укороченный/аномальный READ CAPACITY | `PARTIAL_ENUM` |
| `055_msc_inquiry_invalid_length_01` | некорректная длина INQUIRY-данных | `PARTIAL_ENUM` |
| `056_msc_csw_invalid_status_01` | невалидный/аномальный BOT CSW status, controlled equivalent | `KERNEL_WARNING` |
| `057_msc_stall_on_read10_01` | деградация на READ(10) | `KERNEL_WARNING` |
| `058_msc_endpoint_packet_size_zero_01` | `wMaxPacketSize=0` у MSC endpoint | `KERNEL_WARNING` |
| `059_msc_bot_residue_mismatch_01` | BOT residue mismatch, controlled equivalent | `KERNEL_WARNING` |

## 5. Счётчики по классификации

- `OK`: 1
- `PARTIAL_ENUM`: 6
- `KERNEL_WARNING`: 4
- `EXPECTED_REJECT`: 0
- `DRIVER_BIND_ERROR`: 0
- `USERSPACE_FAILURE`: 0
- `DRIVER_CRASH`: 0
- `SYSTEM_CRASH`: 0

## 6. Основные наблюдения

MSC baseline после MSC-aware runner даёт ожидаемую успешность в повторном прогоне: `050_msc_baseline_repeat_01 -> OK`.

Кейсы `051-055` в целом доходят до USB enumeration и bind к `usb-storage`, но деградируют на storage/SCSI/block-уровне.

Кейсы `056-059` также доходят до USB enumeration и bind к `usb-storage`, при этом фиксируются kernel/storage warnings.

Кейс `058` особенно показателен: `wMaxPacketSize=0` подтверждён напрямую в `lsusb`-артефактах.

Подтверждённых `DRIVER_CRASH` не выявлено. Подтверждённых `SYSTEM_CRASH` не выявлено.

## 7. Ограничения доказательной базы

Часть `journalctl` tail-срезов содержит шум от предыдущих запусков.

Для строгой атрибуции событий к одному запуску нужны более узкие временные окна `journalctl` и желательно `usbmon`/pcap.

Из-за ограничений TinyUSB API часть мутаций BOT/CSW представлена как контролируемые эквиваленты, а не буквальная побайтная порча raw CSW.

## 8. Итог по MSC-блоку

В текущих Linux-прогонах USB/`usb-storage` стек сохранил устойчивость: malformed MSC-поведение приводило к деградации и предупреждениям на уровне хранения, но без подтверждённых падений драйвера или kernel panic/system crash.

## 9. Дальнейшие Linux-only шаги

- захват `usbmon` для каждого прогона;
- более строгие timestamp-окна kernel-логов на один запуск;
- повтор наиболее сильных кейсов: `058`, `059`, а также деградационных `051-055`;
- композитные кейсы MSC+CDC/HID;
- timing/state mutation cases.
