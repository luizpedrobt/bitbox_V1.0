import streamlit as st
import paho.mqtt.client as mqtt
from datetime import datetime
import ssl
import struct
import re
from collections import deque
import time

# ================== CONFIGURAÇÃO DA PÁGINA ==================
st.set_page_config(page_title="Monitor UART", page_icon="📝", layout="centered")

# ================== CONFIG MQTT ==================
BROKER_URL  = "qa717179.ala.us-east-1.emqxsl.com"
BROKER_PORT = 8883
USERNAME = "luizpedrobt"
PASSWORD = "papagaio23"
CA_CERT_PATH = "emqxsl-ca.crt"
# Assinando apenas o tópico da UART agora
TOPICS = [("datalogger/uart/+", 0)]
CLIENT_ID = "datalogger_streamlit_uart"

# ================== CACHE DE DADOS ==================
@st.cache_resource
def init_mqtt_and_data():
    # Aumentei o limite para guardar as últimas 100 mensagens da UART
    data_store = {"uart": deque(maxlen=100)}

    def payload_to_ascii(payload: bytes) -> str:
        return "".join(chr(b) if 32 <= b <= 126 else "." for b in payload)

    def parse_topic(topic: str):
        # Filtra direto para a UART
        m = re.match(r"datalogger/uart/(\d+)", topic)
        if not m:
            return None
        return int(m.group(1))

    def on_connect(client, userdata, flags, rc):
        if rc == 0:
            print("[MQTT] Conectado - Escutando apenas UART")
            for topic, qos in TOPICS:
                client.subscribe(topic, qos)

    def on_message(client, userdata, msg):
        num = parse_topic(msg.topic)
        if num is None or len(msg.payload) < 8:
            return

        # Extrai timestamp e dados
        ts_us = struct.unpack("<Q", msg.payload[:8])[0]
        data = msg.payload[8:]
        host_ts = datetime.now().strftime("%H:%M:%S")

        frame_str = payload_to_ascii(data)
        
        # Formata a linha de log
        log_line = f"[{host_ts}] (Porta {num}) [TS: {ts_us}]: {frame_str}"
        userdata["uart"].append(log_line)

    client = mqtt.Client(client_id=CLIENT_ID, userdata=data_store)
    client.username_pw_set(USERNAME, PASSWORD)
    client.tls_set(ca_certs=CA_CERT_PATH, tls_version=ssl.PROTOCOL_TLSv1_2)
    client.tls_insecure_set(False)
    
    client.on_connect = on_connect
    client.on_message = on_message
    
    client.connect(BROKER_URL, BROKER_PORT, keepalive=60)
    client.loop_start() # Roda em background

    return client, data_store

# ================== INTERFACE STREAMLIT ==================

st.title("📝 Monitor Serial UART")
st.markdown("Acompanhe as mensagens de texto chegando do seu datalogger.")

# Inicializa MQTT e pega a referência dos dados
client, data_store = init_mqtt_and_data()

# Interface do Terminal
if len(data_store["uart"]) > 0:
    # Junta as strings (revertidas para a mais nova ficar no topo)
    uart_text = "\n".join(reversed(data_store["uart"]))
    
    st.text_area("Console de Recepção (Últimas 100 mensagens)", value=uart_text, height=400)
    
    # Botão para limpar a tela
    if st.button("Limpar Console"):
        data_store["uart"].clear()
        st.rerun() # Força a atualização imediata da tela
else:
    st.info("Aguardando pacotes de dados na UART...")

# ================== ATUALIZAÇÃO AUTOMÁTICA ==================
time.sleep(1)
st.rerun()