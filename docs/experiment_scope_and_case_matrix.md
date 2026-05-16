# Область экспериментов и матрица кейсов USB-устойчивости

## Формальная цель
Разработать аппаратно-программный комплекс на базе RP2040 и ESP32-C3 для моделирования аномального поведения USB-устройства и экспериментальной оценки устойчивости USB-стеков Windows и Linux к некорректным, граничным и нестандартным USB-дескрипторам. Оценка должна охватывать отказы перечисления, некорректный bind драйверов, ошибки драйверов, сбои userspace-компонентов, аварийное завершение компонентов и критические системные сбои при наблюдении такого поведения на контролируемом лабораторном стенде.

## Практическая цель
Обеспечить воспроизводимый формат демонстрации ВКР: «подключить устройство -> наблюдать отказ ОС/драйвера или устойчивое корректное отклонение аномального кейса», с безопасным фокусом на descriptor/enumeration/driver robustness.

## Классификация результатов
- `OK`: устройство обработано корректно, ошибок ОС нет.
- `EXPECTED_REJECT`: ОС корректно отклоняет malformed-устройство.
- `PARTIAL_ENUM`: устройство обнаружено частично, но не полностью сконфигурировано.
- `DRIVER_BIND_ERROR`: устройство обнаружено, но class driver не привязался или привязался некорректно.
- `USERSPACE_FAILURE`: отказ/зависание userspace-компонента (`explorer`, `device manager`, `lsusb`, `udisks`, `gvfs`).
- `KERNEL_WARNING`: в ядре есть warning/error, но система стабильна.
- `DRIVER_CRASH`: crash конкретного драйвера/подсистемы.
- `SYSTEM_CRASH`: BSOD/kernel panic/freeze/reboot.

## Текущая матрица descriptor/enumeration кейсов
### Baseline
- `000_baseline_cdc`
- `001_baseline_hid_no_input`
- `002_baseline_composite_cdc_hid`

### Device descriptor anomalies
- `010_device_blength_too_short`
- `011_device_blength_too_long`
- `012_device_unknown_class`
- `013_device_zero_vid_pid`

### Configuration descriptor anomalies
- `020_config_wtotallength_too_small`
- `021_config_wtotallength_too_large`
- `022_config_bnuminterfaces_mismatch`

### Interface/endpoint anomalies
- `030_interface_class_invalid`
- `031_endpoint_wmaxpacketsize_zero`
- `032_endpoint_direction_mismatch`

### String descriptor anomalies
- `040_string_invalid_length`
- `041_string_missing_index`

## Планируемые группы (документирование next step)
### MSC baseline/malformed
- `050_msc_baseline`
- `051_msc_zero_block_size`
- `052_msc_zero_capacity`
- `053_msc_huge_capacity`
- `054_msc_read_capacity_short_response`
- `055_msc_inquiry_invalid_length`
- `056_msc_csw_invalid_status`
- `057_msc_stall_on_read10`
- `058_msc_endpoint_packet_size_zero`
- `059_msc_bot_residue_mismatch`

### Composite/storage weirdness
- `060_composite_cdc_msc_baseline`
- `061_composite_hid_msc_baseline`
- `062_composite_duplicate_interface_number`
- `063_composite_duplicate_endpoint_address`
- `064_composite_iad_mismatch`
- `065_composite_bnuminterfaces_too_many`
- `066_composite_bnuminterfaces_too_few`

### Timing/state mutation
- `070_descriptor_changes_after_reset`
- `071_descriptor_changes_between_get_descriptor_calls`
- `072_slow_control_response`
- `073_stall_on_second_get_descriptor`
- `074_short_control_transfer`
- `075_inconsistent_string_descriptor_between_reads`

## Границы безопасности
Работы ограничены безопасными экспериментами уровня descriptor/enumeration/driver robustness на контролируемом стенде. Реализация вредоносных BadUSB-нагрузок и сценариев выполнения команд на хосте не входит в область проекта.
