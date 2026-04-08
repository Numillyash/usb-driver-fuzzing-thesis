# RP2040 portable environment for WSL + Docker Desktop

Переносимый шаблон окружения для **Waveshare RP2040 Zero** и других RP2040-плат на базе **Pico SDK**.

## Что внутри

- `docker/Dockerfile` — воспроизводимый build image
- `compose.yaml` — запуск dev-контейнера
- `.devcontainer/devcontainer.json` — открытие проекта прямо в VS Code
- `firmware/` — минимальный проект Pico SDK
- `tools/build-container.sh` — сборка из WSL в контейнере
- `tools/shell.sh` — интерактивная shell-сессия в контейнере
- `tools/flash-windows.ps1` — прошивка `.uf2` с Windows-хоста
- `tools/flash-from-wsl.sh` — вызов Windows-прошивки прямо из WSL
- `tools/flash-linux.sh` — прошивка на обычном Linux-хосте

## Архитектура

- **Сборка** выполняется в контейнере.
- **Прошивка** выполняется с хоста через UF2 (`RPI-RP2`).
- Код лучше хранить **внутри Linux FS WSL**, например в `~/work/...`, а не в `C:\...`.

## 0. Что поставить на ноуте

### Windows

1. Обновить WSL:
   ```powershell
   wsl --update
   ```
2. Поставить Docker Desktop.
3. В Docker Desktop включить:
   - `Use WSL 2 based engine`
   - `Settings -> Resources -> WSL Integration -> Ubuntu = ON`

### В Ubuntu (WSL)

```bash
sudo apt update
sudo apt install -y git ca-certificates curl unzip zip make
```

Проверь Docker из WSL:

```bash
docker version
docker compose version
```

## 1. Клонирование/размещение проекта

Храни проект в Linux FS:

```bash
mkdir -p ~/work
cd ~/work
# сюда либо git clone, либо cp -r
```

## 2. Первая сборка

```bash
cd ~/work/rp2040-portable-env
chmod +x tools/*.sh
./tools/build-container.sh
```

Артефакты появятся в `build/`, включая `portable_demo.uf2`.

## 3. Прошивка платы из WSL

1. Зажми **BOOT** на плате.
2. Подключи USB.
3. Отпусти кнопку, когда появится накопитель `RPI-RP2`.
4. Выполни:

```bash
./tools/flash-from-wsl.sh
```

## 4. Прошивка платы напрямую из Windows PowerShell

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\flash-windows.ps1 -Uf2Path .\build\portable_demo.uf2
```

## 5. Вход в контейнер руками

```bash
./tools/shell.sh
```

После входа:

```bash
cmake -S firmware -B build -G Ninja -DPICO_BOARD=waveshare_rp2040_zero -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
```

## 6. VS Code / Dev Containers

Открой папку проекта через VS Code из WSL:

```bash
code .
```

Потом выбери **Reopen in Container**.

## 7. Если понадобится USB/serial/debug из WSL

Базовый шаблон не зависит от USB passthrough. Но если потом захочешь:
- serial из WSL
- Picoprobe/OpenOCD из WSL
- доступ к USB-устройству напрямую

то это делается через `usbipd-win` на Windows и attach в WSL.

## 8. Как менять плату

Для другой поддерживаемой платы просто замени `PICO_BOARD` в скрипте сборки, например:

- `pico`
- `pico_w`
- `waveshare_rp2040_zero`

