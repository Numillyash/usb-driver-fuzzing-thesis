#!/usr/bin/env python3

import argparse
import sys

from protocol.log_decoder import decode_text_line
from protocol.packets import build_text_command
from transport.serial_bridge import SerialBridge, SerialBridgeError


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Minimal host console skeleton for the USB/radio bridge project."
    )
    parser.add_argument("--port", required=True, help="Serial port path, for example /dev/ttyACM0 or COM7")
    parser.add_argument("--baud", type=int, default=115200, help="Serial baud rate")
    parser.add_argument(
        "--command",
        default="ping",
        help="Simple text command to send after opening the port",
    )
    parser.add_argument(
        "--read-lines",
        type=int,
        default=10,
        help="Maximum number of received text lines to print before exit",
    )
    parser.add_argument(
        "--timeout",
        type=float,
        default=1.0,
        help="Serial read timeout in seconds",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    try:
        with SerialBridge(args.port, baudrate=args.baud, timeout=args.timeout) as bridge:
            command = build_text_command(args.command)
            print(f"host_console: opened {args.port} @ {args.baud}")
            print(f"host_console: sending {command.strip()!r}")
            bridge.send_text(command)

            for _ in range(args.read_lines):
                line = bridge.read_line()
                if line is None:
                    continue

                decoded = decode_text_line(line)
                print(decoded)
    except SerialBridgeError as exc:
        print(f"host_console: serial error: {exc}", file=sys.stderr)
        return 1
    except KeyboardInterrupt:
        print("host_console: interrupted", file=sys.stderr)
        return 130

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
