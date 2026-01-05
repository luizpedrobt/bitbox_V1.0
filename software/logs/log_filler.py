# enviar_pacotes.py
import serial
import time

PORT = "/dev/ttyACM1"   # ajuste se necessário
BAUD = 115200

PACKET_SIZE = 1024     # bytes por pacote
PERIOD = 5.0           # segundos

payload_base = b"hello world"

# monta payload exatamente com 1024 bytes
payload = (payload_base * (PACKET_SIZE // len(payload_base) + 1))[:PACKET_SIZE]

with serial.Serial(PORT, BAUD, timeout=5) as s:
    time.sleep(0.1)  # tempo para abrir a porta

    print("Enviando pacotes de 1024 bytes a cada 5s...")

    while True:
        t0 = time.perf_counter()

        s.write(payload)
        s.flush()

        t1 = time.perf_counter()
        print(f"Pacote enviado em {t1 - t0:.6f} s")

        time.sleep(PERIOD)
