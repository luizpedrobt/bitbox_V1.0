import serial
import time

PORT = "/dev/ttyACM0"          # ajuste aqui
BAUD = 115200


# ================= CRC16 (AJUSTE SE NECESSÁRIO) =================
def crc16_ccitt(data: bytes, poly=0x1021, init_val=0xFFFF):
    crc = init_val
    for byte in data:
        crc ^= (byte << 8)
        for _ in range(8):
            if crc & 0x8000:
                crc = (crc << 1) ^ poly
            else:
                crc <<= 1
            crc &= 0xFFFF
    return crc
# ================================================================


# ================= COBS IGUAL AO SEU C =================
def cobs_encode(data: bytes) -> bytes:
    output = bytearray()
    code_index = 0
    output.append(0)      # placeholder do code
    code = 1

    for b in data:
        if b != 0:
            output.append(b)
            code += 1
            if code == 0xFF:
                output[code_index] = code
                code_index = len(output)
                output.append(0)
                code = 1
        else:
            output[code_index] = code
            code_index = len(output)
            output.append(0)
            code = 1

    output[code_index] = code
    return bytes(output)


ser = serial.Serial(PORT, BAUD, timeout=1)

counter = 729

while True:
    # 1️⃣ payload ASCII
    payload = f"hello ({counter})\n".encode()

    # 2️⃣ CRC
    crc = crc16_ccitt(payload)
    payload_crc = payload + crc.to_bytes(2, "big")

    # 3️⃣ COBS
    encoded = cobs_encode(payload_crc)

    # 4️⃣ Delimitador
    frame = encoded + b'\x00'

    ser.write(frame)

    print("Sent:", frame.hex(" "))

    counter += 1
    time.sleep(1)
