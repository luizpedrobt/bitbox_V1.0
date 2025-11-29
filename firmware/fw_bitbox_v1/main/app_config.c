#include <stdint.h>
#include <stdlib.h>
#include "esp_log.h"
#include "app_config.h"
#include "cJSON.h"
#include "portmacro.h"

QueueHandle_t config_queue = NULL;

static const char *TAG = "APP_CONFIG";

/* ----------- STATIC FUNCTION DECLARATIONS --------------*/

static void config_task(void *param);

/* ----------- STATIC FUNCTION SOURCES --------------*/

static void config_task(void *param)
{
    mqtt_msg_t msg;
    
    while(1)
    {
        if(xQueueReceive(config_queue, &msg, portMAX_DELAY))
        {
            ESP_LOGI(TAG, "Recebido no arquivo config: %.*s", msg.size, msg.data);
        }

        free(msg.data);
    }
    
}

void app_config_init()
{
    config_queue = xQueueCreate(10, sizeof(mqtt_msg_t));
    xTaskCreate(config_task, "config_task", 1024, NULL, 10, NULL);
}