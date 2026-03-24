import paho.mqtt.client as mqtt
from datetime import datetime
import ssl
import os
import argparse
import struct
import re

# ================== CONFIG ==================
BROKER_URL  = "qa717179.ala.us-east-1.emqxsl.com"
BROKER_PORT = 8883

USERNAME = "luizpedrobt"
PASSWORD = "papagaio23"

CA_CERT_PATH = "emqxsl-ca.crt"

TOPICS = [
    ("datalogger/gpio/+", 0),
    ("datalogger/uart/+", 0),
]

CLIENT_ID = "datalogger_python_sub"
# ============================================

log_files = {}  # (periph, num) -> filepath


def payload_to_ascii(payload: bytes) -> str:
    return "".join(
        chr(b) if 32 <= b <= 126 else "."
        for b in payload
    )


def parse_topic(topic: str):
    m = re.match(r"datalogger/(gpio|uart)/(\d+)", topic)
    if not m:
        return None, None
    return m.group(1).upper(), int(m.group(2))


def get_log_file(periph, num, base_dir):
    key = (periph, num)
    if key in log_files:
        return log_files[key]

    ts = datetime.now().strftime("%y-%m-%d %H-%M-%S")
    filename = f"{ts}_{periph}_{num}.txt"
    path = os.path.join(base_dir, filename)
    log_files[key] = path
    return path


def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("[MQTT] Conectado")
        for topic, qos in TOPICS:
            client.subscribe(topic, qos)
    else:
        print(f"[MQTT] Falha na conexão rc={rc}")


def on_message(client, userdata, msg):
    host_ts = datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]

    periph, num = parse_topic(msg.topic)
    if periph is None:
        return

    payload = msg.payload
    if len(payload) < 8:
        return  # inválido

    # -------- timestamp do sistema (8 bytes) --------
    ts_us = struct.unpack("<Q", payload[:8])[0]
    data = payload[8:]

    # -------- interpretação por periférico --------
    if periph == "GPIO":
        if len(data) < 2:
            return
        level = data[0]
        edge  = data[1]
        frame_str = f"LEVEL: {level} | EDGE: {edge}"

    elif periph == "UART":
        frame_str = payload_to_ascii(data)

    else:
        return

    line_file = f"[{host_ts}][{periph} {num}] [{ts_us}] {frame_str}"
    line_print = f"[{ts_us}] {frame_str}"

    print(line_print)

    path = get_log_file(periph, num, userdata["log_dir"])
    with open(path, "a", encoding="utf-8") as f:
        f.write(line_file + "\n")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--log-dir",
        default="logs",
        help="Diretório de saída dos logs"
    )
    args = parser.parse_args()

    os.makedirs(args.log_dir, exist_ok=True)

    client = mqtt.Client(
        client_id=CLIENT_ID,
        userdata={"log_dir": args.log_dir}
    )

    client.username_pw_set(USERNAME, PASSWORD)

    client.tls_set(
        ca_certs=CA_CERT_PATH,
        tls_version=ssl.PROTOCOL_TLSv1_2
    )
    client.tls_insecure_set(False)

    client.on_connect = on_connect
    client.on_message = on_message

    print("[MQTT] Conectando...")
    client.connect(BROKER_URL, BROKER_PORT, keepalive=60)
    client.loop_forever()


if __name__ == "__main__":
    main()
