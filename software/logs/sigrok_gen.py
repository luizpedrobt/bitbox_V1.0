import zipfile

# 1 canal digital, 1 MHz
# waveform: 0000111100001111
samples = [0,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1]

# cada byte = uma amostra, bit0 = D0
data = bytes(s & 0x01 for s in samples)

with zipfile.ZipFile("exemplo.sr", "w", zipfile.ZIP_DEFLATED) as z:
    z.writestr("version", "2\n")
    z.writestr(
        "metadata",
        "\n".join([
            "[global]",
            "sigrok version=0.5.2",
            "",
            "[device 1]",
            "samplerate=1000000",
            "total probes=1",
            "unitsize=1",
            "probe1=D0",
            "capturefile=logic-1",
        ]) + "\n"
    )
    z.writestr("logic-1", data)
