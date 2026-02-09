# enviar_pacotes_ascii.py
import serial
import time
import random
import string

PORT = "/dev/ttyACM1"     # ajuste se necessário
BAUD = 230400

MIN_SIZE = 20            # tamanho mínimo da frase (sem o \0)
MAX_SIZE = 900           # tamanho máximo da frase (sem o \0)
PERIOD = 5.0             # segundos entre envios

random.seed()


WORDS = [
    "sistema", "ativo", "falha", "sensor", "temperatura", "pressao",
    "controle", "estado", "motor", "inicializado", "erro", "warning",
    "telemetria", "pacote", "recebido", "enviado", "ok", "operando",
    "bateria", "tensao", "corrente", "frequencia", "uart", "gpio"
]


def generate_sentence(min_len, max_len):
    """
    Gera uma frase ASCII legível com tamanho controlado
    """
    sentence = ""

    while len(sentence) < min_len:
        word = random.choice(WORDS)
        sentence += word + " "

    # corta se passar do máximo
    sentence = sentence[:max_len]

    # remove espaço final se existir
    sentence = sentence.rstrip()

    return sentence


with serial.Serial(PORT, BAUD, timeout=1) as s:
    time.sleep(0.2)  # tempo para estabilizar a porta

    print("Enviando frases ASCII terminadas em \\0 ...")

    while True:
        sentence = generate_sentence(MIN_SIZE, MAX_SIZE)

        payload = sentence.encode("ascii") + b"\x00"

        t0 = time.perf_counter()
        s.write(payload)
        s.flush()
        t1 = time.perf_counter()

        print(f"Frame enviado: {len(payload)-1} bytes + \\0")
        print(f"Conteúdo: \"{sentence}\"")
        print(f"Tempo de envio: {t1 - t0:.6f}s\n")

        time.sleep(PERIOD)
