# ESP32-C3 Docker Build Notes

## Назначение

Этот документ фиксирует минимальный воспроизводимый путь сборки для `firmware/esp32c3_bridge/` через Docker и `docker compose`, не затрагивая существующий RP2040 baseline.

## Что добавлено

- отдельный Docker image для ESP-IDF;
- сервис `esp32c3-dev` в `compose.yaml`;
- минимальный ESP-IDF проект `firmware/esp32c3_bridge/`.

Проект сейчас делает только одно:

- собирается через `idf.py build`;
- печатает простые boot/status сообщения по serial;
- не содержит логики `nRF24` и control-plane runtime.

## Build Commands From WSL

Сборка образа ESP-IDF:

```bash
docker compose build esp32c3-dev
```

Сборка прошивки:

```bash
docker compose run --rm -T esp32c3-dev idf.py build
```

Артефакты ожидаются в каталоге:

```text
firmware/esp32c3_bridge/build/
```

Основной бинарный образ обычно:

```text
firmware/esp32c3_bridge/build/esp32c3_bridge.bin
```

## Interactive Shell

Для ручной работы внутри контейнера:

```bash
docker compose run --rm esp32c3-dev bash
```

Внутри контейнера:

```bash
idf.py build
```

## Host-Side Flash / Monitor Notes

Как и в текущем RP2040 процессе, сборка остаётся внутри Docker, а работа с платой предпочтительно выполняется на хосте.

Типовой host-side поток:

1. Подключить ESP32-C3 плату к Windows.
2. Определить COM-порт в Device Manager.
3. Прошить артефакты с хоста через локально установленный ESP-IDF или `esptool.py`.
4. Открыть serial monitor с хоста.

Пример команд на хосте Windows при установленном ESP-IDF:

```powershell
idf.py -p COM7 flash
idf.py -p COM7 monitor
```

Если используется только `esptool.py`, понадобится указать bootloader, partition table и application image из `build/`.

## Notes For WSL + Docker Desktop

- исходники рекомендуется держать в Linux filesystem внутри WSL;
- `docker compose` запускается из WSL;
- USB/serial passthrough в контейнер не требуется для обычной сборки;
- flash/monitor лучше выполнять с хоста, где COM-порт доступен напрямую.

## Current Scope Limits

- только build baseline для ESP32-C3;
- только serial boot/status messages;
- без радиомоста, без NRF24, без host protocol handler;
- без изменений в RP2040 runtime.
