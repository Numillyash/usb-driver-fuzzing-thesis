# План Linux-only экспериментов Composite USB (060-066)

##
1. Цель и границы

Документ задаёт безопасный план composite-блока экспериментов устойчивости USB-стека Linux для устройств с несколькими интерфейсами, начиная с baseline комбинации CDC ACM + MSC.

Ограничения:
- только RP2040 / Waveshare RP2040 Zero;
- без hardware-запусков, прошивки и flash в рамках данного шага;
- без BadUSB, без автоматического ввода, без выполнения команд на хосте;
- без доступа к дискам хоста и destructive write-поведения.

##
2. Блок кейсов 060-066

| Case ID
| Имя
| Тип кейса
| Краткая цель |
|---|---|---|---|
| 060
| `composite_cdc_msc_baseline`
| baseline
| Валидный composite CDC ACM + read-only RAM-backed MSC |
| 061
| `composite_hid_msc_baseline`
| baseline
| Валидный composite HID (inert) + read-only MSC |
| 062
| `composite_duplicate_interface_number`
| descriptor mutation
| Проверка реакции на дубли интерфейсного номера |
| 063
| `composite_duplicate_endpoint_address`
| descriptor mutation
| Проверка реакции на конфликт адресов endpoint |
| 064
| `composite_iad_mismatch`
| descriptor mutation
| Проверка реакции на неконсистентный IAD |
| 065
| `composite_bnuminterfaces_too_many`
| descriptor mutation
| Проверка реакции на завышенный `bNumInterfaces` |
| 066
| `composite_bnuminterfaces_too_few`
| descriptor mutation
| Проверка реакции на заниженный `bNumInterfaces` |

##
3. Статус реализации

- `060_composite_cdc_msc_baseline`: реализуется как безопасный baseline с persona `composite_cdc_msc`.
- CDC используется только как диагностический/inert transport.
- MSC реализуется как read-only RAM-backed логический носитель с контролируемым отказом записи.
- `061_composite_hid_msc_baseline`: реализован как безопасный baseline с persona `composite_hid_msc`.
- HID-интерфейс инертный generic (без keyboard/mouse/user-control отчётов), MSC остаётся read-only RAM-backed.
- `062_composite_duplicate_interface_number`: реализован как безопасный descriptor-mutation кейс на базе persona `composite_cdc_msc`; в configuration descriptor намеренно дублируется `bInterfaceNumber` (MSC использует уже занятый номер CDC-интерфейса).
- `063_composite_duplicate_endpoint_address`: реализован как безопасный descriptor-mutation кейс на базе persona `composite_cdc_msc`; в configuration descriptor намеренно дублируется `bEndpointAddress` (MSC IN endpoint использует адрес CDC IN endpoint).
- `064_composite_iad_mismatch`: реализован как безопасный descriptor-mutation кейс на базе persona `composite_cdc_msc`; в CDC IAD намеренно задан неконсистентный `bFirstInterface`, не совпадающий с фактической раскладкой CDC-интерфейсов в configuration descriptor.
- `065_composite_bnuminterfaces_too_many`: реализован как безопасный descriptor-mutation кейс на базе persona `composite_cdc_msc`; в configuration descriptor поле `bNumInterfaces` намеренно завышено относительно фактического числа interface descriptor.
- Документы результатов для `docs/results/060_*` создаются только после аппаратного прогона.
