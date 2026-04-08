# usb-driver-fuzzing-thesis

Основной приватный репозиторий ВКР по теме аппаратного фаззинга и исследований USB-драйверов.

## Что это

Репозиторий объединяет:

- переносимое окружение сборки для RP2040 на базе **WSL Ubuntu + Docker Desktop + Pico SDK**;
- firmware для микроконтроллерных плат, используемых в экспериментах;
- вспомогательные скрипты сборки, прошивки и запуска;
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
3. **Experiment layer** — сценарии фаззинга, тестовые кейсы, артефакты экспериментов.
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
├─ .devcontainer/
│  └─ devcontainer.json
├─ firmware/
│  ├─ CMakeLists.txt
│  └─ src/
├─ tools/
│  ├─ build-container.sh
│  ├─ shell.sh
│  ├─ flash-from-wsl.sh
│  ├─ flash-windows.ps1
│  └─ flash-linux.sh
├─ docs/
│  ├─ thesis/
│  ├─ notes/
│  └─ references/
├─ experiments/
├─ results/
├─ scripts/
├─ compose.yaml
├─ .gitignore
└─ README.md
```

## Быстрый старт

### 1. Сборка

```bash
./tools/build-container.sh
```

### 2. Прошивка RP2040 через BOOTSEL

Подключить плату в режиме `BOOTSEL`, затем:

```bash
./tools/flash-from-wsl.sh
```

### 3. Проверка runtime

После прошивки устройство должно подняться как USB CDC serial, а тестовая прошивка — выводить heartbeat-сообщения.

## Ближайшие шаги

- превратить `portable_demo` в полноценный smoke-test firmware;
- добавить команды `help`, `ping`, `info`, `reboot` по serial;
- добавить LED/self-test для платы;
- перенести в репозиторий реальный код экспериментов ВКР;
- оформить разделы `docs/`, `experiments/`, `results/`;
- при необходимости подключить CI для headless build.

## Статус

Репозиторий находится в активной инженерной и исследовательской разработке.
