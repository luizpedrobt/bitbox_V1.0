#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "mqtt_app.h"
#include "mqtt_client.h"

#include "cJSON.h"
#include "app_config.h"
#include "esp_log.h"
#include "uart_periph.h"
#include "esp_system.h"

#define MQTT_BROKER_URL "mqtts://qa717179.ala.us-east-1.emqxsl.com"
#define MQTT_BROKER_PORT 8883

extern const uint8_t server_cert_start[] asm("_binary_emqxsl_ca_crt_start");
extern const uint8_t server_cert_end[]   asm("_binary_emqxsl_ca_crt_end");

static const char *TAG = "MQTT";

/* ----------- STATIC FUNCTION DECLARATIONS --------------*/

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

static void mqtt_topic_filter(const char *topic, const char *payload);

#define X(topic_ref, topic_name, topic_size, topic_handle_func) static void topic_handle_func(const char *suffix, const char *payload);
    XMACRO_TOPIC_HANDLER_SUB_LISTS
#undef X

static bool parse_uart_json(const char *json, uart_cfg_t *cfg);

/* ----------- TOPIC HANDLERS DECLARATIONS --------------*/

static const topic_handler_t topic_handlers[] = 
{
    #define X(topic_ref, topic_name, topic_size, topic_handle_func) {topic_name, topic_size, topic_handle_func},
        XMACRO_TOPIC_HANDLER_SUB_LISTS
    #undef X
};

/* ----------- STATIC FUNCTION SOURCES --------------*/

static void mqtt_topic_filter(const char *topic, const char *payload)
{
    for(uint8_t i = 0; i < MAX_TOPICS; i++)
    {
        if(strncmp(topic, topic_handlers[i].topic, topic_handlers[i].topic_len) == 0)
        {
            topic_handlers[i].func(topic + topic_handlers[i].topic_len, payload);
        }
    }
}

static void handle_config_message(const char *suffix, const char *payload)
{
    ESP_LOGI(TAG, "Sufixo: %s| Dado: %s", suffix, payload);

    uart_cfg_t config;
    parse_uart_json(payload, &config);

    uart_set_new_configure(&config);
}

static void handle_ota_message(const char *suffix, const char *payload)
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
            
            for(uint8_t i = 0; i < MAX_TOPICS; i++)
            {
                msg_id = esp_mqtt_client_subscribe(client, topic_handlers[i].topic, 0);
                ESP_LOGI(TAG, "Subscrito no tópico %s ! msg_id=%d", topic_handlers[i].topic, msg_id);
            }
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            msg_id = esp_mqtt_client_publish(client, "topic/config", "AA FOLOU A FOLOU IU", 0, 0, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
            break;

        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_DATA:    
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");

            ESP_LOGI(TAG, "TOPIC=[%.*s]\r\n", event->topic_len, event->topic);
            ESP_LOGI(TAG, "DATA=%s | SIZE = %d\r\n", event->data, event->data_len);

            char topic[128];
            char data[256];

            snprintf(topic, sizeof(topic), "%.*s", event->topic_len, event->topic);
            snprintf(data, sizeof(data), "%.*s", event->data_len, event->data);

            mqtt_msg_t msg;

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
    
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

    esp_mqtt_client_start(client);
}

static bool parse_uart_json(const char *json, uart_cfg_t *cfg)
{   
    cJSON *root = cJSON_Parse(json);
    if(!root)
        return false;

    cfg->uart_num = cJSON_GetObjectItem(root, "uart_num")->valueint;
    cfg->tx_pin = cJSON_GetObjectItem(root, "tx_gpio")->valueint;
    cfg->rx_pin = cJSON_GetObjectItem(root, "rx_gpio")->valueint;
    cfg->baudrate = cJSON_GetObjectItem(root, "baudrate")->valueint;

    cJSON_Delete(root);
    return true;        
}