import time
import ssl
import paho.mqtt.client as mqtt
import json
import random

# --- CONFIGURAÇÕES (Preencha com seus dados) ---
BROKER_URL = "qa717179.ala.us-east-1.emqxsl.com" # Seu endereço do print anterior
BROKER_PORT = 8883                                # Porta segura obrigatória
USERNAME = "luizpedrobt"                   # O mesmo que você criou na aba Authentication
PASSWORD = "papagaio23"
TOPIC = "topic/config"                          # Tópico onde o ESP32 vai escutar
CA_CERT_PATH = "components/mqtt_app/certs/emqxsl-ca.crt"                  # Nome do arquivo que você baixou (ou .pem)

# --- CALLBACKS ---
def on_connect(client, userdata, flags, rc, properties=None):
    if rc == 0:
        print(f"[OK] Conectado ao Broker MQTT com sucesso!")
    else:
        print(f"[ERRO] Falha ao conectar. Código de retorno: {rc}")

def on_publish(client, userdata, mid, reason_code=None, properties=None):
    print(f"[TX] Mensagem enviada (ID: {mid})")

# --- SETUP DO CLIENTE ---
# Usando a API v2 para compatibilidade com versões novas do Paho
client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)

# Autenticação
client.username_pw_set(USERNAME, PASSWORD)

# Configuração de Segurança (TLS/SSL) - Obrigatório para EMQX Serverless
# context = ssl.create_default_context() # Opcional: Contexto customizado se precisar
client.tls_set(ca_certs=CA_CERT_PATH, 
               cert_reqs=ssl.CERT_REQUIRED, 
               tls_version=ssl.PROTOCOL_TLSv1_2)

# Vincula as funções de callback
client.on_connect = on_connect
client.on_publish = on_publish

# --- LOOP PRINCIPAL ---
try:
    print(f"Conectando a {BROKER_URL}:{BROKER_PORT}...")
    client.connect(BROKER_URL, BROKER_PORT, 60)
    
    # Inicia o loop em uma thread separada para manter a conexão viva
    client.loop_start()

    while True:
        # Criando um payload fake (JSON) simulando um comando ou dado
        payload = {
            "temperatura": round(random.uniform(20.0, 30.0), 2),
            "comando": "LED_ON",
            "timestamp": time.time()
        }
        
        mensagem = json.dumps(payload)
        
        # Publica no tópico
        info = client.publish(TOPIC, mensagem, qos=2)
        info.wait_for_publish() # Garante que saiu do PC
        
        print(f"Enviado: {mensagem}")
        
        time.sleep(5) # Envia a cada 5 segundos

except KeyboardInterrupt:
    print("\nDesconectando...")
    client.loop_stop()
    client.disconnect()

except Exception as e:
    print(f"\nErro Crítico: {e}")
    print("Dica: Verifique se o caminho do certificado (CA_CERT_PATH) está correto.")