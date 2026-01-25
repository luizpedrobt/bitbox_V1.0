import struct

INPUT_FILE = "decoded/gpio3.bin"
OUTPUT_FILE = "decoded/gpio3.txt"

REC_FMT = "<Q B B"
REC_SIZE = struct.calcsize(REC_FMT)

with open(INPUT_FILE, "rb") as f:
    data = f.read()

with open(OUTPUT_FILE, "w") as out:
    for i in range(0, len(data), REC_SIZE):
        ts, edge, level = struct.unpack_from(REC_FMT, data, i)
        out.write(
            f"[{i//REC_SIZE:05d}] {ts} us | edge={edge} level={level}\n"
        )

print("GPIO TXT gerado.")