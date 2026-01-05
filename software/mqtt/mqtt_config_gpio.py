import json
import time
import ssl
import argparse
import paho.mqtt.client as mqtt

# --- CONFIGURAÇÕES MQTT ---
BROKER_URL = "qa717179.ala.us-east-1.emqxsl.com"
BROKER_PORT = 8883
USERNAME = "luizpedrobt"
PASSWORD = "papagaio23"
TOPIC = "topic/config/gpio"
CA_CERT_PATH = "emqxsl-ca.crt"

# --- ENUMERAÇÕES ESP-IDF ---
GPIO_MODE = {
    "DISABLE": 0,
    "INPUT": 1,
    "OUTPUT": 2,
    "INPUT_OUTPUT": 3,
}

GPIO_PULL = {
    "DISABLE": 0,
    "ENABLE": 1,
}

GPIO_INTR = {
    "DISABLE": 0,
    "POSEDGE": 1,
    "NEGEDGE": 2,
    "ANYEDGE": 3,
    "LOW_LEVEL": 4,
    "HIGH_LEVEL": 5,
}

# --- GPIOS PERMITIDOS ---
VALID_GPIOS =      {1, 2, 3, 4, 5, 33, 34, 35, 37, 38}
VALID_GPIOS_ENUM = {0, 1, 2, 3, 4,  5,  6,  7,  8,  9}

# ---------- CLI ----------
parser = argparse.ArgumentParser(description="Publica configuração de GPIO via MQTT")

parser.add_argument(
    "--gpio",
    type=int,
    nargs="+",
    required=True,
    help="GPIO(s) a configurar (ex: 1 2 33 38)"
)

parser.add_argument(
    "--mode",
    choices=GPIO_MODE.keys(),
    default="INPUT",
    help="Modo do GPIO"
)

parser.add_argument(
    "--intr",
    choices=GPIO_INTR.keys(),
    default="ANYEDGE",
    help="Tipo de interrupção"
)

parser.add_argument(
    "--pullup",
    choices=GPIO_PULL.keys(),
    default="ENABLE",
    help="Pull-up"
)

parser.add_argument(
    "--pulldown",
    choices=GPIO_PULL.keys(),
    default="DISABLE",
    help="Pull-down"
)

parser.add_argument(
    "--state",
    type=int,
    choices=[0, 1],
    default=1,
    help="Habilita (1) ou desabilita (0) o GPIO"
)

args = parser.parse_args()

# ---------- VALIDAÇÃO ----------
for gpio in args.gpio:
    if gpio not in VALID_GPIOS:
        raise ValueError(f"GPIO inválido: {gpio}")

# ---------- MQTT ----------
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Conectado ao broker MQTT")
    else:
        print(f"Erro MQTT (rc={rc})")

client = mqtt.Client()
client.username_pw_set(USERNAME, PASSWORD)

client.tls_set(
    ca_certs=CA_CERT_PATH,
    tls_version=ssl.PROTOCOL_TLSv1_2
)

client.connect(BROKER_URL, BROKER_PORT, keepalive=60)
client.loop_start()
time.sleep(1)

# ---------- PUBLICAÇÃO ----------
for gpio in args.gpio:
    payload = {
        "state": args.state,
        "gpio_num": gpio,
        "mode": GPIO_MODE[args.mode],
        "pull_up_en": GPIO_PULL[args.pullup],
        "pull_down_en": GPIO_PULL[args.pulldown],
        "intr_type": GPIO_INTR[args.intr]
    }

    print(f"Publicando GPIO{gpio}: {json.dumps(payload)}")
    client.publish(TOPIC, json.dumps(payload), qos=1)
    time.sleep(0.3)

client.loop_stop()
client.disconnect()
