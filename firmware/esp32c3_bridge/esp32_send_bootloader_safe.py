import serial
import time

port = "COM3"

with serial.Serial(port, 115200, timeout=0.2, write_timeout=1.0) as ser:
    time.sleep(2.0)

    print("[initial]")
    end = time.time() + 2.0
    while time.time() < end:
        if ser.in_waiting:
            print(ser.read(ser.in_waiting).decode(errors="replace"), end="")
        time.sleep(0.05)

    for cmd in ["help", "bootloader"]:
        print(f"\n>>> {cmd}")
        ser.write((cmd + "\r\n").encode())
        ser.flush()

        end = time.time() + 3.0
        while time.time() < end:
            if ser.in_waiting:
                print(ser.read(ser.in_waiting).decode(errors="replace"), end="")
            time.sleep(0.05)

print("\nDone.")
