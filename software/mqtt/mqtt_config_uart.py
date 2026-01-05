import json
import time
import ssl
import paho.mqtt.client as mqtt

# --- CONFIGURAÇÕES (já preenchidas) ---
BROKER_URL = "qa717179.ala.us-east-1.emqxsl.com"
BROKER_PORT = 8883
USERNAME = "luizpedrobt"
PASSWORD = "papagaio23"
TOPIC = "topic/config/uart"
CA_CERT_PATH = "emqxsl-ca.crt"

# --- MENSAGENS A SEREM PUBLICADAS ---
messages = [
    {
        "state": 0,
        "uart_num": 0,
        "rx_gpio": 1,
        "tx_gpio": 2,
        "baudrate": 115200
    },
    {
        "state": 0,
        "uart_num": 1,
        "rx_gpio": 3,
        "tx_gpio": 4,
        "baudrate": 57600
    },
    {
        "state": 0,
        "uart_num": 2,
        "rx_gpio": 33,
        "tx_gpio": 34,
        "baudrate": 9600
    }
]

# --- CALLBACKS ---
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Conectado com sucesso ao broker MQTT")
    else:
        print(f"Falha na conexão, rc={rc}")

def on_publish(client, userdata, mid):
    print(f"Mensagem publicada (mid={mid})")

# --- CLIENTE MQTT ---
client = mqtt.Client()
client.username_pw_set(USERNAME, PASSWORD)

client.on_connect = on_connect
client.on_publish = on_publish

client.tls_set(
    ca_certs=CA_CERT_PATH,
    tls_version=ssl.PROTOCOL_TLSv1_2
)

client.tls_insecure_set(False)

client.connect(BROKER_URL, BROKER_PORT, keepalive=60)
client.loop_start()

time.sleep(1)

# --- PUBLICAÇÃO ---
for msg in messages:
    payload = json.dumps(msg)
    print(f"Publicando: {payload}")
    client.publish(TOPIC, payload, qos=1, retain=False)
    time.sleep(1)

time.sleep(2)

client.loop_stop()
client.disconnect()
