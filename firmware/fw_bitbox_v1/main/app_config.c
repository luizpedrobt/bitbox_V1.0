#include <stdint.h>
#include <stdlib.h>
#include "esp_log.h"
#include "app_config.h"
#include "cJSON.h"
#include "portmacro.h"

QueueHandle_t config_msg_queue = NULL;

static const char *TAG = "APP_CONFIG";

void app_config_init()
{
    config_msg_queue = xQueueCreate(5, sizeof(config_msg_t));
}

static void config_task(void *param)
{
    config_msg_t msg;
    
    while(1)
    {
        if(xQueueReceive(config_msg_queue, &msg, portMAX_DELAY))
        {
            ESP_LOGI(TAG, "Recebido no arquivo config: %.*s", msg.len, msg.payload);
        }

        free(msg.payload);
    }
    
}