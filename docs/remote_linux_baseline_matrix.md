# Матрица базовых проверок USB-persona на удалённом Linux-стенде

## Назначение

Документ фиксирует результаты первых безопасных baseline-проверок USB-устройства на базе RP2040. Проверки выполнялись на удалённом Linux-хосте через автоматизированный цикл: генерация конфигурации кейса, сборка UF2, перевод RP2040 в BOOTSEL через ESP32-C3/nRF24, прошивка UF2, авторизация USB-устройства на Linux-хосте и сбор USB-артефактов.

## Проверенные baseline-кейсы

### 000_baseline_cdc

Кейс: `experiments/cases/000_baseline_cdc.json`.

Результат: успешно.

Подтверждённые признаки:
- устройство определяется как `2e8a:000a Raspberry Pi Pico`;
- создаётся CDC ACM интерфейс;
- драйвер Linux привязывает устройство к `cdc_acm`;
- появляется `/dev/ttyACM0`;
- serial smoke-test проходит: `help`, `info`, `ping`;
- RF-подсистема активна: `rf_ready=1`.

Артефакты:
- `results/raw/linux/000_baseline_cdc_remote_success_*`;
- `results/raw/linux/000_baseline_cdc_repeat_*`;
- `results/raw/linux/000_baseline_cdc_repeat_05_fix_*`.

### 001_baseline_hid_no_input

Кейс: `experiments/cases/001_baseline_hid_no_input.json`.

Результат: успешно.

Подтверждённые признаки:
- устройство определяется как `2e8a:4005 RP2040 USB Research`;
- устройство имеет один HID-интерфейс;
- HID-дескриптор виден через `lsusb -v`;
- Linux привязывает устройство к `hid-generic`;
- устройство инертное: не реализует клавиатурный ввод, мышиные события, BadUSB-поведение или команды на стороне хоста.

Артефакты:
- `results/raw/linux/001_baseline_hid_no_input_01_*`.

### 002_baseline_composite_cdc_hid

Кейс: `experiments/cases/002_baseline_composite_cdc_hid.json`.

Результат: успешно.

Подтверждённые признаки:
- устройство определяется как `2e8a:4005 RP2040 USB Research usb_case_custom_demo Composite`;
- в device descriptor указан IAD-compatible режим: `bDeviceClass = Miscellaneous Device`, `bDeviceProtocol = Interface Association`;
- конфигурация содержит три интерфейса:
  - interface 0: CDC Communication;
  - interface 1: CDC Data;
  - interface 2: HID;
- Linux привязывает CDC часть к `cdc_acm`;
- Linux привязывает HID часть к `hid-generic`;
- появляется `/dev/ttyACM0`;
- HID остаётся инертным и не выполняет host-side действия.

Артефакты:
- `results/raw/linux/002_baseline_composite_cdc_hid_01_*`.

## Итог

Удалённый Linux-стенд готов к безопасным descriptor/enumeration экспериментам. Подтверждены:
- удалённая перепрошивка RP2040 без физического BOOTSEL;
- автоматизированный запуск кейса через `tools/run-remote-linux-case.sh`;
- сбор Linux USB-артефактов;
- работоспособность CDC, HID и composite CDC+HID baseline-профилей.

Следующий этап — запуск контролируемых аномальных descriptor-кейсов из диапазона `010–041` с фиксацией реакции Linux-хоста.
