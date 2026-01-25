#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "mqtt_app.h"
#include "esp_timer.h"

#include "mqtt_client.h"

#include "cJSON.h"
#include "esp_log.h"
#include "uart_periph.h"
#include "gpio_peripheral.h"

#include "sd_log.h"

#include "embled_app.h"

#include "esp_system.h"

#define DEVICE_ID 1

#define MQTT_BROKER_URL "mqtts://qa717179.ala.us-east-1.emqxsl.com"
#define MQTT_BROKER_PORT 8883

static int embl_app_pins[PORT_MAX_LEDS] =
{
    GPIO_NUM_17, GPIO_NUM_18,
};

extern const uint8_t server_cert_start[] asm("_binary_emqxsl_ca_crt_start");
extern const uint8_t server_cert_end[]   asm("_binary_emqxsl_ca_crt_end");

static esp_mqtt_client_handle_t mqtt_client = NULL;

typedef struct PACKED mqtt_msg_s
{
    uint64_t time_us;
    union
    {
        sd_log_uart_t uart;
        sd_log_gpio_t gpio;
    };
} mqtt_msg_t;

static const char *TAG = "MQTT";

static bool mqtt_connected = false;

/* ----------- STATIC FUNCTION DECLARATIONS --------------*/

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

static void mqtt_topic_filter(const char *topic, const char *payload);

#define X(topic_ref, topic_name, topic_size, topic_handle_func) static void topic_handle_func##_func(const char *suffix, const char *payload);
    XMACRO_TOPIC_HANDLER_SUB_LISTS
#undef X

static bool parse_uart_json(const char *json, uart_cfg_t *cfg);

static bool parse_gpio_json(const char *json, gpio_cfg_t *cfg);

/* ----------- TOPIC HANDLERS DECLARATIONS --------------*/

static const topic_handler_t topic_handlers[] = 
{
    #define X(topic_ref, topic_name, topic_size, topic_handle_func) {topic_name, topic_size, topic_handle_func##_func},
        XMACRO_TOPIC_HANDLER_SUB_LISTS
    #undef X
};

/* ----------- STATIC FUNCTION SOURCES --------------*/

static void mqtt_topic_filter(const char *topic, const char *payload)
{
    for(uint8_t i = 0; i < MAX_TOPICS; i++)
    {
        uint16_t len = topic_handlers[i].topic_len;

        if (strncmp(topic, topic_handlers[i].topic, len) == 0 && topic[len] == '/')
        {
            topic_handlers[i].func(topic + len, payload);
            return;
        }
    }
}

static void handle_config_message_func(const char *suffix, const char *payload)
{
    ESP_LOGI(TAG, "SUFFIX=[%s]", suffix);

    if(strcmp(suffix, "/uart") == 0)
    {
        uart_cfg_t uart_config;

        parse_uart_json(payload, &uart_config);
        uart_set_new_configure(&uart_config);
        ESP_LOGI(TAG, "Nova mensagem de configuração de UART!");
    }

    else if(strcmp(suffix, "/gpio") == 0)
    {
        gpio_cfg_t gpio_config;

        parse_gpio_json(payload, &gpio_config);
        gpio_set_new_configure(&gpio_config);
        ESP_LOGI(TAG, "Nova mensagem de configuração de GPIO!");
    }

    else
    {
        ESP_LOGW(TAG, "Mensagem de configuração não suportada!");
    }
}

static void handle_ota_message_func(const char *suffix, const char *payload)
{
    ESP_LOGI(TAG, "Sufixo: %s| Dado: %s", suffix, payload);
}

static void handle_cmd_message_func(const char *suffix, const char *payload)
{
    ESP_LOGI(TAG, "Sufixo: %s| Dado: %s", suffix, payload);
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);

    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;

    int msg_id;

    switch ((esp_mqtt_event_id_t)event_id)
    {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            mqtt_connected = true;
            
            for(uint8_t i = 0; i < MAX_TOPICS; i++)
            {
                msg_id = esp_mqtt_client_subscribe(client, topic_handlers[i].topic, 0);
                ESP_LOGI(TAG, "Subscrito no tópico %s ! msg_id=%d", topic_handlers[i].topic, msg_id);
            }
            break;

        case MQTT_EVENT_DISCONNECTED:
            mqtt_connected = false;

            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_DATA:    
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");

            char topic[128];
            char data[256];

            snprintf(topic, sizeof(topic), "%.*s", event->topic_len, event->topic);
            snprintf(data, sizeof(data), "%.*s", event->data_len, event->data);

            ESP_LOGI(TAG, "TOPIC=[%s]", topic);

            mqtt_topic_filter(topic, data);

            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
                ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
            
            break;

        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
}

void mqtt_deinit_app(void)
{
    ESP_LOGI(TAG, "Cliente MQTT finializado!");
    esp_mqtt_client_stop(mqtt_client);
}

void mqtt_main_app(void)
{
    const esp_mqtt_client_config_t mqtt_cfg =
    {
        .broker.address.uri = MQTT_BROKER_URL,
        .broker.address.port = MQTT_BROKER_PORT,
        .broker.verification.certificate = (const char *)server_cert_start,

        .credentials.username = "luizpedrobt",
        .credentials.authentication.password = "papagaio23",
    };
    
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

    esp_mqtt_client_start(mqtt_client);
}

bool mqtt_publish_msg(const sd_log_msg_t *msg)
{
    if (!msg)
    {
        return false;
    }

    char topic[64];
    uint8_t payload[8 + UART_MAX_PAYLOAD_LEN + 1];
    size_t payload_len = 0;

    switch (msg->log_header.log_type)
    {
        /* ================= UART ================= */
        case SD_LOG_UART:
        {
            snprintf(topic, sizeof(topic), "datalogger/uart/%d", msg->log_header.periph_num);

            uint8_t *p = payload;

            memcpy(p, &msg->log_header.time_us, sizeof(uint64_t));
            p += sizeof(uint64_t);

            memcpy(p, msg->uart.payload, msg->uart.payload_len);
            p += msg->uart.payload_len;

            *p++ = '\0';

            payload_len = p - payload;

            esp_mqtt_client_publish(mqtt_client, topic, (const char *)payload, payload_len, 0, 0);

            embled_set_mode(embl_app_pins[PORT_STATUS], EMBLED_DRIVER_MODE_DIGITAL, EMBLED_MODE_PULSE_TRIPLE, EMBLED_ACTIVE_HIGH, false);

            return true;
        }

        /* ================= GPIO ================= */
        case SD_LOG_GPIO:
        {
            snprintf(topic, sizeof(topic), "datalogger/gpio/%d" , msg->log_header.periph_num);

            uint8_t *p = payload;

            memcpy(p, &msg->log_header.time_us, sizeof(uint64_t));
            p += sizeof(uint64_t);

            *p++ = msg->gpio.edge;
            *p++ = msg->gpio.level;

            payload_len = p - payload;

            esp_mqtt_client_publish(mqtt_client, topic,(const char *)payload,payload_len,1,0);

            embled_set_mode(embl_app_pins[PORT_OPER], EMBLED_DRIVER_MODE_DIGITAL, EMBLED_MODE_PULSE_DOUBLE, EMBLED_ACTIVE_HIGH, false);

            return true;
        }

        default:
            ESP_LOGW(TAG, "Tipo de log não suportado");
            return false;
    }
}

static bool parse_uart_json(const char *json, uart_cfg_t *cfg)
{   
    cJSON *root = cJSON_Parse(json);
    if(!root)
        return false;

    cfg->state = cJSON_GetObjectItem(root, "state")->valueint;
    cfg->uart_num = cJSON_GetObjectItem(root, "uart_num")->valueint;
    cfg->tx_pin = cJSON_GetObjectItem(root, "tx_gpio")->valueint;
    cfg->rx_pin = cJSON_GetObjectItem(root, "rx_gpio")->valueint;
    cfg->baudrate = cJSON_GetObjectItem(root, "baudrate")->valueint;

    cJSON_Delete(root);
    return true;        
}

static bool parse_gpio_json(const char *json, gpio_cfg_t *cfg)
{
    cJSON *root = cJSON_Parse(json);
    if(!root)
        return false;

    cfg->state = cJSON_GetObjectItem(root, "state")->valueint;
    cfg->mode = cJSON_GetObjectItem(root, "mode")->valueint;
    cfg->gpio_num = cJSON_GetObjectItem(root, "gpio_num")->valueint;
    cfg->pull_up_en = cJSON_GetObjectItem(root, "pull_up_en")->valueint;
    cfg->pull_down_en = cJSON_GetObjectItem(root, "pull_down_en")->valueint;
    cfg->intr_type = cJSON_GetObjectItem(root, "intr_type")->valueint;

    cJSON_Delete(root);
    return true;    
}