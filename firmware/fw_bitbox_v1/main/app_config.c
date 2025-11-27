#include <stdint.h>
#include <stdlib.h>
#include "esp_log.h"
#include "app_config.h"
#include "cJSON.h"
#include "portmacro.h"

QueueHandle_t config_msg = NULL;

static const char *TAG = "APP_CONFIG";

void app_config_init()
{
    config_msg = xQueueCreate(10, sizeof(mqtt_msg_t));
}

static void config_task(void *param)
{
    mqtt_msg_t msg;
    
    while(1)
    {
        if(xQueueReceive(config_msg, &msg, portMAX_DELAY))
        {
            ESP_LOGI(TAG, "Recebido no arquivo config: %.*s", msg.size, msg.data);
        }

        free(msg.data);
    }
    
}