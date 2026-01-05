import struct
import os

# ========= CONFIGURAÇÕES =========

INPUT_FILE = "LOG1.BIN"
OUTPUT_DIR = "decoded"

HEADER_MAGIC = 0xDEADBEEF
UART_MAX_PAYLOAD_LEN = 1000   # ajuste conforme seu firmware

# Tipos (ajuste se seus enums forem diferentes)
SD_LOG_UART = 0x01
SD_LOG_GPIO = 0x02

# Struct formats (little-endian)
UART_STRUCT_FMT = f"<I Q B B H {UART_MAX_PAYLOAD_LEN}s"
GPIO_STRUCT_FMT = "<I Q B B B B"

UART_STRUCT_SIZE = struct.calcsize(UART_STRUCT_FMT)
GPIO_STRUCT_SIZE = struct.calcsize(GPIO_STRUCT_FMT)

os.makedirs(OUTPUT_DIR, exist_ok=True)

# Arquivos abertos por periférico
uart_files = {}
gpio_files = {}

# ========= FUNÇÕES =========

def get_uart_file(uart_num):
    if uart_num not in uart_files:
        fname = os.path.join(OUTPUT_DIR, f"uart{uart_num}.bin")
        uart_files[uart_num] = open(fname, "ab")
    return uart_files[uart_num]

def get_gpio_file(gpio_num):
    if gpio_num not in gpio_files:
        fname = os.path.join(OUTPUT_DIR, f"gpio{gpio_num}.bin")
        gpio_files[gpio_num] = open(fname, "ab")
    return gpio_files[gpio_num]

# ========= DECODER =========

with open(INPUT_FILE, "rb") as f:
    data = f.read()

offset = 0
total_len = len(data)

while offset + 4 <= total_len:
    header = struct.unpack_from("<I", data, offset)[0]

    if header != HEADER_MAGIC:
        offset += 1
        continue

    # Tenta UART
    if offset + UART_STRUCT_SIZE <= total_len:
        try:
            unpacked = struct.unpack_from(UART_STRUCT_FMT, data, offset)
            _, time_us, log_type, periph_num, msg_len, msg_data = unpacked

            if log_type == SD_LOG_UART and msg_len <= UART_MAX_PAYLOAD_LEN:
                out = get_uart_file(periph_num)
                out.write(msg_data[:msg_len])
                offset += UART_STRUCT_SIZE
                continue
        except struct.error:
            pass

    # Tenta GPIO
    if offset + GPIO_STRUCT_SIZE <= total_len:
        try:
            unpacked = struct.unpack_from(GPIO_STRUCT_FMT, data, offset)
            _, time_us, type_, periph_num, edge, level = unpacked

            if type_ == SD_LOG_GPIO:
                out = get_gpio_file(periph_num)
                out.write(struct.pack("<Q B B", time_us, edge, level))
                offset += GPIO_STRUCT_SIZE
                continue
        except struct.error:
            pass

    # Se não reconheceu nada, anda 1 byte
    offset += 1

# ========= FECHAMENTO =========

for f in uart_files.values():
    f.close()

for f in gpio_files.values():
    f.close()

print("Decodificação concluída.")
