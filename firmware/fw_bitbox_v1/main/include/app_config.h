#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef struct config_msg_s
{
    char *payload;
    int len;
}config_msg_t;

extern QueueHandle_t config_msg_queue;

void app_config_init();