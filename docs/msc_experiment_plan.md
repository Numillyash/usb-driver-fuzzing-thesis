# План Linux-only экспериментов MSC (050-059)

## 1. Цель и границы

Документ задаёт безопасный план следующего блока экспериментов устойчивости USB-стека Linux для класса Mass Storage Class (MSC, BOT), без реализации вредоносного поведения и без выхода за рамки descriptor/enumeration + контролируемых class-level ответов.

Ограничения выполнения:
- только стенд RP2040 / Waveshare RP2040 Zero;
- без запуска hardware-экспериментов в рамках данного шага;
- без прошивки устройств и без flash;
- без доступа к дискам хоста;
- без автоматического выполнения команд на хосте;
- без реализации destructive write-сценариев.

## 2. Текущее состояние инфраструктуры

По состоянию репозитория:
- USB-case инфраструктура уже поддерживает переключение persona через `USB_CASE_ID` и генерацию метаданных кейса из `experiments/cases/*.json` (`tools/gen_usb_case_config.py`).
- Для кастомных дескрипторов используется target `usb_case_custom_demo` с TinyUSB (`USB_CASE_ENABLE_TINYUSB_CUSTOM_DESCRIPTORS=1`, `USB_CASE_MANUAL_TINYUSB_INIT=1`).
- В `tusb_config.h` сейчас заданы `CFG_TUD_CDC=1`, `CFG_TUD_HID=0` (по умолчанию) и нет включения MSC.

Вывод: для MSC-блока нужен отдельный безопасный persona-слой в `usb_case_custom_demo` (или отдельный target), изолированный от baseline `portable_demo`.

## 3. Безопасный MSC baseline persona

### 3.1 Принцип persona

Предлагаемый baseline: `msc_inert_readonly`.

Свойства:
- однофункциональное USB MSC-устройство (без HID/keyboard/mouse);
- логический носитель только in-memory (RAM-backed), фиксированного малого размера;
- запрет записи со стороны хоста (read-only режим на уровне SCSI/MSC callbacks);
- отсутствие проксирования к реальным блочным устройствам хоста.

### 3.2 Почему это безопасно

- нет выполнения кода на хосте;
- нет поставки исполняемых payload;
- поведение ограничено протоколом BOT/SCSI-ответов;
- write-операции переводятся в controlled reject (ошибка/STALL по безопасной политике), без изменения состояния хоста.

## 4. Требуемые TinyUSB-конфигурации

Минимально требуемые флаги/параметры для MSC-блока:
- `CFG_TUD_MSC=1`;
- `CFG_TUD_CDC=0` для чистого MSC baseline (допустимо оставить `=1` только для отдельной composite-ветки, но не как базовый MSC-кейс);
- `CFG_TUD_HID=0`;
- `CFG_TUD_ENDPOINT0_SIZE=64` (как в текущем baseline);
- `CFG_TUD_MSC_EP_BUFSIZE` в безопасном диапазоне (обычно 64 или 512; для FS-экспериментов достаточно консервативного значения, согласованного с TinyUSB).

Требование изоляции:
- не менять сборочную работоспособность `portable_demo`;
- добавление MSC-флагов делать только для нового USB-case таргета/конфигурации.

## 5. Структура дескрипторов для MSC

Для baseline `050_msc_baseline`:
- Device descriptor: допустим class-level вариант `bDeviceClass = 0x00` (класс на уровне интерфейса) либо корректный composite-friendly вариант, но без лишних интерфейсов.
- Configuration descriptor:
  - `bNumInterfaces = 1`;
  - интерфейс MSC: `bInterfaceClass = 0x08`, `bInterfaceSubClass = 0x06` (SCSI transparent), `bInterfaceProtocol = 0x50` (BOT);
  - два bulk endpoint: `OUT` и `IN`, корректные `wMaxPacketSize` для FS.
- String descriptors: консистентные, без пропущенных индексов.

Для кейсов 051-059 мутации вносятся относительно этого baseline.

## 6. Нужные runtime callbacks (TinyUSB MSC)

Планируемый минимальный набор callbacks:
- `tud_msc_inquiry_cb`;
- `tud_msc_test_unit_ready_cb`;
- `tud_msc_capacity_cb`;
- `tud_msc_start_stop_cb`;
- `tud_msc_read10_cb`;
- `tud_msc_write10_cb`;
- `tud_msc_scsi_cb` (для контролируемой обработки/инъекции отдельных аномалий в целевых кейсах).

Требования к реализации callbacks:
- детерминированность ответов;
- явная телеметрия в лог (какая команда, какой результат);
- возможность case-id-зависимых мутаций без изменения baseline-пути.

## 7. Политика inert/read-only

Обязательные меры:
- write-path по умолчанию запрещён (`write10` -> reject/failed status по безопасной схеме);
- `read10` возвращает данные только из локального RAM-буфера (предзаполненный шаблон);
- фиксированный небольшой объём устройства (для baseline);
- отсутствие любого маппинга на внешние накопители, файловые системы или ресурсы хоста.

Дополнительно:
- при невозможности корректно поддержать write для тестов BOT — использовать постоянный read-only семантический режим и отражать это в `INQUIRY`/sense-кодах.

## 8. Linux evidence: что фиксировать

Для каждого прогона 050-059 фиксировать минимум:
- `lsusb -v` snapshot до/после подключения;
- `dmesg -w` или `journalctl -k -f` окно событий на момент enumeration/bind;
- `usb-devices` итоговый снимок;
- `lsblk -o NAME,MODEL,SIZE,RO,TYPE,MOUNTPOINT` (проверка появления устройства и флага `RO`);
- статус userspace storage-слоя (`udisksctl status` при наличии);
- runtime-лог прошивки/раннера (идентификатор кейса, persona, callback telemetry).

Желательно для усиления доказательной базы:
- `usbmon`/pcap контрольных прогонов (как минимум baseline 050 и 1-2 аномальных кейса).

## 9. Предлагаемые кейсы 050-059

## 9.1 Матрица кейсов

| Case ID | Имя | Тип мутации | Ожидаемая реакция Linux | Ожидаемая классификация |
|---|---|---|---|---|
| 050 | `msc_baseline` | Корректный MSC BOT read-only baseline | Успешное перечисление и bind `usb-storage` без kernel warning | `OK` |
| 051 | `msc_zero_block_size` | В capacity `block_size=0` | Отказ инициализации логического диска | `EXPECTED_REJECT` или `PARTIAL_ENUM` |
| 052 | `msc_zero_capacity` | `block_count=0` | Возможна частичная инициализация без usable media | `PARTIAL_ENUM` |
| 053 | `msc_huge_capacity` | Нереалистично большой `block_count` | Возможен reject или ограниченный bind с warning | `PARTIAL_ENUM` или `KERNEL_WARNING` |
| 054 | `msc_read_capacity_short_response` | Укороченный ответ `READ CAPACITY` | Ошибка протокола BOT/SCSI, отказ инициализации | `EXPECTED_REJECT` |
| 055 | `msc_inquiry_invalid_length` | Неконсистентная длина `INQUIRY` data | Ошибка парсинга inquiry | `EXPECTED_REJECT` или `PARTIAL_ENUM` |
| 056 | `msc_csw_invalid_status` | Некорректный `CSW` status | BOT reset/recovery, возможный отказ bind | `PARTIAL_ENUM` или `KERNEL_WARNING` |
| 057 | `msc_stall_on_read10` | STALL на `READ(10)` | Ошибка I/O при обращении к блочному устройству | `PARTIAL_ENUM` |
| 058 | `msc_endpoint_packet_size_zero` | `wMaxPacketSize=0` у bulk endpoint | Ранний reject endpoint descriptor | `EXPECTED_REJECT` |
| 059 | `msc_bot_residue_mismatch` | Несогласованный residue в `CSW` | Протокольная ошибка, recovery/отказ | `PARTIAL_ENUM` или `KERNEL_WARNING` |

## 9.2 Границы допустимости для кейсов

- Каждый кейс должен быть безопасным и не должен включать payload-логику.
- Мутации ограничиваются дескрипторами и BOT/SCSI-ответами.
- Никакие кейсы не должны писать данные вне RAM-буфера устройства.

## 10. Ожидаемые классификационные исходы

Целевое распределение (гипотеза для Linux):
- `050` -> `OK`.
- `051, 054, 058` -> преимущественно `EXPECTED_REJECT`.
- `052, 055, 056, 057, 059` -> преимущественно `PARTIAL_ENUM`.
- `053, 056, 059` могут дать `KERNEL_WARNING` без `SYSTEM_CRASH`.

Критически важный критерий безопасности эксперимента:
- отсутствие `SYSTEM_CRASH` как целевого результата и отсутствие попыток его провоцирования вредоносным методом.

## 11. Риски реализации и меры контроля

Основные риски:
- риск сломать текущий `portable_demo`/quick-start при изменении глобальных TinyUSB флагов;
- риск неявного write-path в MSC callbacks;
- риск смешения baseline и аномальных веток в одном коде без чёткой case-gating логики;
- риск недостаточной доказательности без `usbmon`.

Меры снижения:
- изолировать MSC-функциональность в отдельном persona-слое/таргете;
- не менять существующие tools/quick-start сценарии;
- зафиксировать unit-level проверки callback-поведения (минимум логические проверки на read-only политику);
- сохранять единый шаблон артефактов для Linux-run.

## 12. Порядок следующего шага (без реализации в этом документе)

1. Добавить JSON-описания кейсов `050-059` в `experiments/cases/` и прогнать `tools/validate_usb_cases.py`.
2. Подготовить безопасную реализацию MSC baseline persona в отдельной ветке firmware (без затрагивания `portable_demo`).
3. Провести Linux-only прогоны по методике и собрать артефакты.
4. Оформить результаты в `docs/results/` по одному файлу на кейс + обновить общую сводку.

В рамках текущей задачи изменения firmware не выполняются.

## 13. Статус реализации

- `050_msc_baseline`: реализован безопасный baseline `msc_inert_readonly` с отдельным target `usb_case_msc_demo`.
- Транспорт: TinyUSB MSC с RAM-backed носителем малого размера и политикой read-only (запись отклоняется).
- `051_msc_zero_block_size`: реализован безопасный case-gated runtime-вариант для `USB_CASE_ID=51`, где в `tud_msc_capacity_cb` возвращается `block_size=0`; baseline `050` оставлен без изменений.
- `052_msc_zero_capacity`: реализован безопасный case-gated runtime-вариант для `USB_CASE_ID=52`, где в `tud_msc_capacity_cb` возвращается `block_count=0` (нулевая usable capacity) при сохранении inert/read-only модели; baseline `050/051` оставлены без изменений.
