# Каталог кейсов

Каталог содержит начальный набор безопасных кейсов для исследования реакций USB-стека Windows и Linux.

## Формат

Кейсы хранятся в формате JSON (без YAML) для обеспечения совместимости со стандартной библиотекой Python и снижения внешних зависимостей.

## Группы

- `baseline` — контрольные корректные профили.
- `device_descriptor` — мутации device descriptor.
- `config_descriptor` — мутации configuration descriptor.
- `interface_endpoint` — мутации interface/endpoint descriptor.
- `string_descriptor` — мутации string descriptor.

## Правила безопасности

- Запрещены полезные нагрузки выполнения команд на хосте.
- Запрещены сценарии эскалации привилегий и закрепления.
- Разрешены только контролируемые аномалии descriptor/enumeration.

## Валидация кейсов

Для проверки корректности и безопасности всех JSON-кейсов используйте скрипт:

```bash
python3 tools/validate_usb_cases.py
```

Что проверяется автоматически:
- наличие обязательных полей (`case_id`, `name`, `group`, `base_persona`, `description`, `mutation`, `expected_windows`, `expected_linux`, `risk_level`, `notes`);
- совпадение `case_id` с префиксом имени файла (например, `010_...json` -> `"case_id": "010"`);
- допустимость `risk_level` (`safe`, `low`, `medium`);
- отсутствие запрещённых формулировок в строковых полях (`reverse shell`, `powershell payload`, `net user`, `credential theft`, `persistence`, `keylogger`).

Код возврата:
- `0` — все кейсы прошли проверку;
- `1` — есть ошибки валидации;
- `2` — инфраструктурная ошибка (например, отсутствует каталог кейсов).
