import serial
import time

port = "COM3"

with serial.Serial(port, 115200, timeout=0.2, write_timeout=1.0) as ser:
    time.sleep(1.0)

    for i in range(10):
        print(f">>> bootloader #{i+1}")
        ser.write(b"bootloader\r\n")
        ser.flush()

        end = time.time() + 0.7
        while time.time() < end:
            if ser.in_waiting:
                print(ser.read(ser.in_waiting).decode(errors="replace"), end="")
            time.sleep(0.05)

print("\nDone.")
