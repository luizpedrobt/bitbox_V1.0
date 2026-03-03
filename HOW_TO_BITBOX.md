# BitBox IoT Datalogger - Documentação Técnica Completa

**Versão do Firmware:** 1.0  
**Plataforma:** ESP32 (ESP-IDF v5.x)

Este documento detalha a operação, configuração, protocolos de comunicação e ferramentas de host para o sistema BitBox. O sistema opera como um datalogger híbrido (SD Card + MQTT) com capacidades de controle remoto e configuração dinâmica via Portal Captivo.

---

## Índice

1. [Arquitetura de Hardware](#1-arquitetura-de-hardware)
2. [Modos de Operação e Boot](#2-modos-de-operação-e-boot)
3. [Provisionamento (Captive Portal)](#3-provisionamento-captive-portal)
4. [Protocolo MQTT (MQTTS)](#4-protocolo-mqtt-mqtts)
5. [Sistema de Arquivos e Logs (SD Card)](#5-sistema-de-arquivos-e-logs-sd-card)
6. [Ferramentas de Host (Python Scripts)](#6-ferramentas-de-host-python-scripts)
7. [Proteções e Robustez](#7-proteções-e-robustez)

---

## 1. Arquitetura de Hardware

### 1.1. Mapeamento de GPIOs
O firmware abstrai os pinos físicos do ESP32 através de "Índices Lógicos" (0 a 9) para facilitar a configuração remota.

| Índice Lógico | Pino Físico (ESP32) / Label na Placa |
| :---: | :---: | 
| **0** | GPIO 1 | 
| **1** | GPIO 2 | 
| **2** | GPIO 3 | 
| **3** | GPIO 4 | 
| **4** | GPIO 5 | 
| **5** | GPIO 33 | 
| **6** | GPIO 34 | 
| **7** | GPIO 35 | 
| **8** | GPIO 36 | 
| **9** | GPIO 37 | 

### 1.2. Botões de Sistema
Interrupções (ISR) configuradas com Debounce e proteção `IRAM_ATTR`.

| GPIO | Label na Placa | Função | Comportamento |
| :--- | :--- | :--- | :--- |
| **41** | SW4 | **Setup WiFi** | Pressionar no boot ou runtime para abrir Portal WiFi. |
| **42** | SW5 | **Setup Hardware** | Pressionar no boot ou runtime para abrir Portal de Hardware. |
| **0** | SW3 | **Reset/Factory Reset** | Pressionamento curto para resetar. Segurar por **3 segundos** para apagar NVS e reiniciar. |

---

## 2. Modos de Operação e Boot

O sistema decide seu modo de operação baseado na existência de configurações na NVS (Non-Volatile Storage) e no estado dos botões.

1.  **Normal Boot:**
    * Carrega configurações de WiFi, UART e GPIO da NVS.
    * Inicializa drivers apenas para periféricos marcados como `Enabled`.
    * Conecta ao MQTT e monta o SD Card.
    * Inicia Tasks de Log.

2.  **Config Mode (Portal):**
    * Ativado via botões GPIO 41 ou 42.
    * Sobe um Access Point (AP).
    * Disponibiliza servidor Web para configuração.
    * Também é possível realizar configuração de pinos via terminal com um script em python + arquivo .yaml.

3.  **Factory Reset:**
    * Apaga todas as chaves da namespace `storage`.
    * Reinicia o dispositivo limpo.

---

## 3. Provisionamento (Captive Portal)

O dispositivo cria uma rede Wi-Fi local para configuração.

* **SSID:** `DEADBEEF_BitBoxV1_CFG`
* **Senha:** (Aberta/Sem senha)
* **IP do Gateway:** `192.168.4.1`

### 3.1. Portal de Rede (Botão WiFi - GPIO 41)
Interface para conectar o dispositivo à internet.
* **Funcionalidades:** Scan de redes (AJAX/Fetch API), Seleção de SSID, Input de Senha.
* **Endpoint de Scan:** `GET /scan` (Retorna JSON com RSSI e SSIDs).

### 3.2. Portal de Hardware (Botão HW - GPIO 42)
Interface dinâmica para configurar periféricos. O formulário HTML adapta-se para enviar apenas configurações de itens habilitados.

#### Configuração UART (0 a 2)
* **Enable Checkbox:** Se desmarcado, libera recursos do ESP32.
* **Baudrate:** Padrão 115200.
* **TX/RX Pins:** Permite mapear qualquer GPIO válido (Matrix Map).

#### Configuração GPIO (0 a 9)
* **Mode:** Input ou Output.
* **Resistors:** Pull-Up, Pull-Down ou Floating.
* **Initial State:** LOW ou HIGH (apenas para Output).

---

## 4. Protocolo MQTT (MQTTS)

A comunicação utiliza **SSL/TLS (Porta 8883)**. O certificado CA do servidor (`emqxsl-ca.crt`) está embarcado no firmware.

### 4.1. Tópicos de Publicação (Device -> Cloud)
O dispositivo envia dados binários para otimizar largura de banda.

| Periférico | Tópico | Estrutura do Payload (Binário) |
| :--- | :--- | :--- |
| **UART** | `datalogger/uart/{N}` | `[Time(8B)]` + `[Payload]` + `[\0] ou [\n] ou [\r]` |
| **GPIO** | `datalogger/gpio/{N}` | `[Time(8B)]` + `[Edge(1B)]` + `[Level(1B)]` |

* **Time:** `uint64_t` (microssegundos desde o boot).
* **Payload:** Dados brutos da UART. Se exceder 1024 bytes, é fragmentado.

### 4.2. Tópicos de Subscrição (Comandos Remotos)
O dispositivo aceita comandos em formato **JSON**.

#### A. Controle de GPIO
* **Tópico:** `datalogger/cmd/gpio`
* **Payload JSON:**
    ```json
    {
      "pin_index": 0,    // Índice lógico (0..9)
      "level": 1         // 0 ou 1
    }
    ```
    > **Segurança:** O comando é ignorado se o pino não estiver habilitado ou configurado como INPUT.
    > **Chamada do script:** python remote_cmd.py gpio 0 1

#### B. Envio via UART
* **Tópico:** `datalogger/cmd/uart`
* **Payload JSON:**
    ```json
    {
      "uart_num": 1,
      "msg": "AT+RESET\r\n"
    }
    ```
    > **Thread Safety:** O envio é protegido por Mutex (`uart_tx_mutex`) para evitar colisão com envios automáticos.
    > **Chamada do script:** python remote_cmd.py uart 1 "Teste de envio"

---

## 5. Sistema de Arquivos e Logs (SD Card)

Os logs são salvos em formato binário proprietário para máxima velocidade e menor uso de CPU.

* **Arquivo:** `LOGxx.BIN` (Incremento automático no boot).
* **Header Magic:** `0xDEADBEEF`

### Estrutura do Frame Binário

1.  **Cabeçalho Geral (14 Bytes):**
    * `Magic` (4B): `0xDEADBEEF`
    * `Timestamp` (8B): `uint64_t` microssegundos.
    * `LogType` (1B): `1` (UART) ou `2` (GPIO).
    * `PeriphNum` (1B): ID do periférico (0-2 para UART, 0-9 para GPIO).

2.  **Dados UART (Variável):**
    * `Length` (2B): `uint16_t` tamanho do payload.
    * `Payload` (N Bytes): Dados brutos.
    * `Terminator` (1B): `\0`, `\n`, `\r`

3.  **Dados GPIO (2 Bytes):**
    * `Edge` (1B): Borda detectada.
    * `Level` (1B): Nível lógico atual.

    > **Chamada do script:** python process_log.py LOGXX.BIN -> Extrai os logs .bin e .txt de cada periférico na pasta padrão `decoded_logs/`. \\

    > **Chamada do script:** python process_log.py LOG05.BIN --out analise_teste_campo -> Extrai os logs .bin e .txt de cada periférico na pasta .`analise_teste_campo`. \\

    > **Chamada do script:** python process_log.py LOG12.BIN --clean-> Extrai apenas .txt de cada periférico na pasta padrão `decoded_logs/`. \\

    > **Chamada do script (Testado em Linux):** for f in *.BIN; do python process_log.py "$f" --clean; done -> extrai todos os logs do diretório atual. \\
---

## 6. Ferramentas de Host (Python Scripts)

Scripts localizados na pasta `software/` para automação de testes e análise.

### 6.1. Controlador Remoto (`remote_cmd.py`)
Interface CLI para enviar comandos MQTT seguros.

**Instalação:**
```bash
pip install paho-mqtt
