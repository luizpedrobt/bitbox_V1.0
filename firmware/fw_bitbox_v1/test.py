# enviar_100k.py
import serial
import time

PORT = "/dev/ttyACM1"   # ajuste (Windows: COM3, Linux: /dev/ttyUSB0)
BAUD = 115200
BYTES_TO_SEND = 210_000  # 200k bytes

data = b'A' * BYTES_TO_SEND

with serial.Serial(PORT, BAUD, timeout=5) as s:
    time.sleep(0.1)  # dar tempo para abrir porta
    t0 = time.perf_counter()
    s.write(data)
    s.flush()
    t1 = time.perf_counter()

print(f"Enviados {BYTES_TO_SEND} bytes em {t1 - t0:.6f} s (lado remetente)")
