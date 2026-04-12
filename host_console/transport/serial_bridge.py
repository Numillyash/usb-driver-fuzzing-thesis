from __future__ import annotations

from dataclasses import dataclass
from typing import Optional

try:
    import serial
    from serial import SerialException
except ImportError:  # pragma: no cover - dependency may be absent locally
    serial = None

    class SerialException(Exception):
        pass


class SerialBridgeError(RuntimeError):
    pass


@dataclass
class SerialBridge:
    port: str
    baudrate: int = 115200
    timeout: float = 1.0

    def __post_init__(self) -> None:
        self._serial = None

    def __enter__(self) -> "SerialBridge":
        self.open()
        return self

    def __exit__(self, exc_type, exc, tb) -> None:
        self.close()

    def open(self) -> None:
        if serial is None:
            raise SerialBridgeError("pyserial is not installed; run `pip install pyserial`")

        try:
            self._serial = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                timeout=self.timeout,
            )
        except SerialException as exc:
            raise SerialBridgeError(f"failed to open {self.port}: {exc}") from exc

    def close(self) -> None:
        if self._serial is not None:
            self._serial.close()
            self._serial = None

    def send_text(self, command: str) -> int:
        if self._serial is None:
            raise SerialBridgeError("serial port is not open")

        data = command.encode("utf-8")
        return self._serial.write(data)

    def read_line(self) -> Optional[str]:
        if self._serial is None:
            raise SerialBridgeError("serial port is not open")

        raw = self._serial.readline()
        if not raw:
            return None

        return raw.decode("utf-8", errors="replace").rstrip("\r\n")
