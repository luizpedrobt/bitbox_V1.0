#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef struct mqtt_msg_s
{
    char *data;
    int size;
}mqtt_msg_t;

// extern QueueHandle_t config_queue; // Fila de Configuração
// extern QueueHandle_t cmd_queue;    // Fila de Comandos Rápidos (GPIO)
// extern QueueHandle_t ota_queue;    // Fila de Atualização de Firmware

void app_config_init();