# RP2040 Flash Layout Draft

## Назначение

Ниже зафиксирован первый черновик разметки внешней flash для стенда на RP2040. Разметка нужна для разделения firmware, сценариев, конфигурации и локального журнала. Это проектная схема, а не реализованный storage driver.

## Предпосылки

- базовая плата: RP2040 Zero;
- типичный объём flash в целевом варианте: 2 MiB;
- erase sector: 4 KiB;
- program page: 256 B;
- XIP firmware и данные разделяют один и тот же flash chip.

Если на конкретной плате объём flash отличается, layout должен масштабироваться через compile-time constants.

## Цели разметки

- не мешать обычной загрузке firmware и `portable_demo`;
- выделить стабильную область под сценарии;
- отделить редко меняемую конфигурацию от интенсивно пишущегося лога;
- оставить явные границы для recovery и миграции layout version.

## Черновая карта адресов

Для flash размером `0x200000` байт:

| Region | Offset | Size | Назначение |
| --- | --- | --- | --- |
| Firmware / XIP image | `0x000000` | `0x140000` (1280 KiB) | `boot2`, код, rodata, TinyUSB и запас под рост прошивки |
| Scenario store | `0x140000` | `0x060000` (384 KiB) | сохранённые сценарии и их метаданные |
| Config region | `0x1A0000` | `0x010000` (64 KiB) | активная конфигурация, counters, boot policy, panic marker |
| Log ring | `0x1B0000` | `0x050000` (320 KiB) | циклический журнал событий |

Итог: используется весь объём `0x200000` байт.

## Region Details

### Firmware / XIP image

- начинается с `0x000000`;
- содержит обычный RP2040 boot flow;
- не должен пересекаться с пользовательским storage;
- верхняя граница выбрана с запасом, чтобы не ломать текущий `portable_demo` build path.

### Scenario store

Назначение:

- хранение нескольких сценариев с метаданными;
- хранение descriptor variants, timing profiles и связанных blob-объектов;
- доступ по slot table или object directory.

Черновая организация:

- region header;
- slot directory;
- scenario objects;
- padding для выравнивания по sector boundary.

### Config region

Назначение:

- одна активная конфигурация устройства;
- boot counters;
- panic / watchdog markers;
- layout version и migration state.

Черновая стратегия записи:

- dual-copy или journaled key-value layout;
- всегда минимум одна валидная копия конфигурации;
- запись только sector-granular commit-процедурой.

### Log ring

Назначение:

- локальный write-heavy журнал;
- хранение событий до выгрузки по radio/serial;
- сохранение следов при зависании тестовой Windows-машины.

Черновая стратегия:

- append-only records;
- wrap-around при достижении конца региона;
- отдельный superblock с write pointer / generation;
- логические записи выравниваются минимум до 4 байт.

## Suggested Headers

Каждый region должен начинаться с компактного заголовка:

```c
typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint16_t version;
    uint16_t header_size;
    uint32_t region_size;
    uint32_t crc32;
} flash_region_header_t;
```

Сигнатуры:

- firmware region: использует стандартный RP2040 image layout;
- scenario store: `0x53434E52` (`SCNR`);
- config region: `0x43464752` (`CFGR`);
- log ring: `0x4C4F4752` (`LOGR`).

## Suggested Safety Rules

- любые region offsets кратны `0x1000`;
- runtime не пишет в firmware region;
- erase log ring не должен затрагивать config region;
- layout version проверяется на boot до старта `AUTORUN`;
- при несогласованности headers устройство стартует в `SAFE`.

## Future Scaling

Для 4 MiB flash layout может быть расширен так:

- сохранить firmware region на `0x140000` или увеличить до `0x180000`;
- scenario store расширить первым делом;
- log ring расширить вторым делом;
- config region оставить небольшим, но дублированным.

## Open Questions

- нужен ли отдельный region под crash dump или RF mailbox;
- хранить ли сценарии как фиксированные slots или как object store;
- нужна ли дедупликация descriptor blobs;
- какой объём log ring минимально достаточен для длительных автономных прогонов;
- где хранить monotonic experiment counter и policy flags для safe boot.
