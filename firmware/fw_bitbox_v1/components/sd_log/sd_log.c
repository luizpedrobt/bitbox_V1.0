#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/queue.h"

#include "sd_log.h"
#include "uart_periph.h"
#include "sdmmc_storage.h"
#include "esp_err.h"
#include "esp_log.h"

#include "esp_timer.h"

#define SD_LOG_QUEUE_DEPTH 64

static QueueHandle_t sd_log_queue;

static const char *TAG = "SD_LOG";

bool sd_log_uart_data(sd_log_uart_t *msg)
{
    if(!msg || !sd_log_queue)
    {
        ESP_LOGE(TAG, "Mensagem incompleta ou fila não inválida!");
        return false;
    }

    sd_log_msg_t log = { 0 };

    log.type = SD_LOG_UART;

    log.uart.header = msg->header; 
    log.uart.time_us = msg->time_us;
    log.uart.log_type = msg->log_type;
    log.uart.periph_num = msg->periph_num;
    log.uart.msg_len = msg->msg_len;

    if(log.uart.msg_len > UART_MAX_PAYLOAD_LEN)
    {   
        ESP_LOGW(TAG, "Tamanho máximo de mensagem ultrapassado!");
        log.uart.msg_len = UART_MAX_PAYLOAD_LEN;
    }

    memcpy(log.uart.msg_data, msg->msg_data, log.uart.msg_len);

    if(xQueueSend(sd_log_queue, &log, 0) != pdTRUE)
    {
        ESP_LOGE(TAG, "Erro ao enviar token de fila!");
        return false;
    }

    ESP_LOGI(TAG, "Mensagem gravada com sucesso!");

    return true;
}

bool sd_log_gpio_data(sd_log_gpio_t *msg)
{
    if(!msg || !sd_log_queue)
    {
        ESP_LOGE(TAG, "Mensagem incompleta ou fila não inválida!");
        return false;
    }

    sd_log_msg_t log = { 0 };

    log.type = SD_LOG_GPIO;

    log.gpio.header = msg->header;
    log.gpio.time_us = msg->time_us;
    log.gpio.type = msg->type;
    log.gpio.periph_num = msg->periph_num;
    log.gpio.edge = msg->edge;
    log.gpio.level = msg->level;

    if(xQueueSend(sd_log_queue, &log, 0) != pdTRUE)
    {
        ESP_LOGE(TAG, "Erro ao enviar token de fila!");
        return false;
    }

    return true;
}

static void sd_log_task(void *arg)
{
    sd_log_msg_t msg;

    while(1)
    {
        if(xQueueReceive(sd_log_queue, &msg, pdMS_TO_TICKS(2000)) == pdTRUE)
        {
            switch (msg.type)
            {
                case SD_LOG_UART:
                    sdmmc_stor_record_data_bin(&msg.uart, sizeof(msg.uart));
                    ESP_LOGI(TAG, "%d bytes gravados pela UART%d!", msg.uart.msg_len, msg.uart.periph_num);
                    break;

                case SD_LOG_GPIO:
                    sdmmc_stor_record_data_bin(&msg.gpio, sizeof(msg.gpio));
                    ESP_LOGI(TAG, "Estado do GPIO%d gravado com sucesso!", msg.gpio.periph_num);
                    break;
                
                default:
                    ESP_LOGE(TAG, "Tipo de log desconhecido!");
                    break;
            }
        }

        else
        {
            ESP_LOGI(TAG, "Esperando dados!");
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void sd_log_main(void)
{
    sd_log_queue = xQueueCreate(SD_LOG_QUEUE_DEPTH, sizeof(sd_log_msg_t));
    xTaskCreate(sd_log_task, "log_task", 4096, NULL, 9, NULL);
}
