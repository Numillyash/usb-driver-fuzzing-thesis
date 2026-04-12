# usb-driver-fuzzing-thesis

Основной приватный репозиторий ВКР по теме аппаратного фаззинга и исследований USB-драйверов.

## Что это

Репозиторий объединяет:

- переносимое окружение сборки для RP2040 на базе **WSL Ubuntu + Docker Desktop + Pico SDK**;
- firmware для микроконтроллерных плат, используемых в экспериментах;
- host-side tooling для сборки, прошивки и запуска окружения;
- материалы ВКР, заметки, постановки экспериментов и результаты.

Текущий базовый контур уже проверен end-to-end:

- сборка firmware в Docker-контейнере;
- выпуск `.uf2`-артефакта;
- прошивка платы с хоста;
- загрузка прошивки на **Waveshare RP2040 Zero**;
- подъем USB CDC serial и успешный runtime smoke-test.

## Текущее состояние

На текущем этапе репозиторий используется как **основная исследовательская и инженерная база** для ВКР. В нем будет развиваться несколько направлений одновременно:

1. **Firmware layer** — прошивки для RP2040 и, при необходимости, других плат.
2. **Tooling layer** — reproducible build environment, devcontainer, Docker, host-side tooling.
3. **Experiment layer** — сценарии фаззинга, тестовые кейсы и артефакты экспериментов.
4. **Documentation layer** — материалы ВКР, заметки, методика, промежуточные выводы.

## Архитектура окружения

Принятая схема:

- **сборка** выполняется в Docker;
- **исходники** хранятся в Linux filesystem внутри WSL;
- **прошивка** выполняется с хоста через UF2/BOOTSEL;
- **USB passthrough в контейнер не требуется**.

Это даёт:

- воспроизводимую сборку;
- переносимость между машинами;
- минимизацию зависимости от локально установленного toolchain;
- простой и устойчивый процесс прошивки RP2040.

## Проверенный baseline

На стенде уже подтверждено:

- `docker version` из WSL работает;
- `docker compose version` работает;
- контейнерная сборка проходит;
- `pico-sdk` корректно настраивается под `waveshare_rp2040_zero`;
- генерируется `portable_demo.uf2`;
- прошивка через host-side script проходит успешно;
- плата определяется в Windows как `COM`-устройство;
- runtime smoke-test выдаёт строку:

```text
portable_demo: RP2040 Zero is alive
```

## Структура репозитория

```text
usb-driver-fuzzing-thesis/
├─ docker/
│  └─ Dockerfile
├─ firmware/
│  ├─ CMakeLists.txt
│  ├─ include/
│  └─ src/
├─ tools/
│  ├─ build-container.sh
│  ├─ shell.sh
│  ├─ flash-from-wsl.sh
│  ├─ flash-windows.ps1
│  └─ flash-linux.sh
├─ docs/
│  ├─ ARCHITECTURE.md
│  ├─ protocol.md
│  ├─ flash_layout.md
│  └─ usb_logging_radio_context_full.md
├─ compose.yaml
└─ README.md
```

## Быстрый старт

### 1. Сборка

```bash
./tools/build-container.sh
```

Артефакт после успешной сборки:

```text
build/portable_demo.uf2
```

### 2. Прошивка RP2040 через BOOTSEL

Подключить плату в режиме `BOOTSEL`, затем:

```bash
./tools/flash-from-wsl.sh
```

### 3. Проверка runtime

После прошивки устройство должно подняться как USB CDC serial, а тестовая прошивка — выводить heartbeat-сообщения.

### 4. Вход в контейнер окружения

```bash
./tools/shell.sh
```

## Текущее наполнение firmware

Сейчас в репозитории сохранён минимальный проверенный target `portable_demo`:

- `firmware/src/main.c` — базовый USB CDC smoke-test;
- `firmware/include/` — типы и черновые интерфейсы control plane, сценариев и логирования;
- `firmware/CMakeLists.txt` — сборка `portable_demo` под `waveshare_rp2040_zero`.

Новая документация и заголовки описывают следующую стадию развития, но не внедряют runtime-логику и не меняют текущий smoke-test.

## Документация

- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) — общая архитектура стенда;
- [docs/protocol.md](docs/protocol.md) — первый черновик control-plane протокола;
- [docs/flash_layout.md](docs/flash_layout.md) — первый черновик разметки flash RP2040;
- [docs/usb_logging_radio_context_full.md](docs/usb_logging_radio_context_full.md) — расширенный контекст по логированию и радиомосту.

## Ближайшие шаги

- превратить `portable_demo` в полноценный smoke-test firmware;
- зафиксировать wire format control plane и storage layout;
- добавить команды `help`, `ping`, `info`, `reboot` по serial;
- добавить LED/self-test для платы;
- перенести в репозиторий реальный код экспериментов ВКР;
- при необходимости подключить CI для headless build.

## Статус

Репозиторий находится в активной инженерной и исследовательской разработке.
