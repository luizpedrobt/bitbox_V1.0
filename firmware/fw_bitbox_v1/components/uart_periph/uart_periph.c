#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "esp_system.h"
#include "freertos/idf_additions.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/uart.h"

#include "esp_timer.h"

#include "uart_periph.h"
#include "gpio_peripheral.h"
#include "sd_log.h"

#include "portmacro.h"
#include "utl_cbf.h"

#include "app_config.h"

static const char *TAG = "UART_PERIPH";

UTL_CBF_DECLARE(uart0_cbf, 10000 * 2);
UTL_CBF_DECLARE(uart1_cbf, 10000 * 2);
UTL_CBF_DECLARE(uart2_cbf, 10000 * 2);

static utl_cbf_t *uart_circ_buffers[UART_NUM_MAX] =
{
    &uart0_cbf,
    &uart1_cbf,
    &uart2_cbf
};

static bool uart_installeds[UART_NUM_MAX] =
{
    false, false, false,
};

static TaskHandle_t uart_task_handlers[UART_NUM_MAX] = { NULL, NULL, NULL };

QueueHandle_t uart_queue[UART_NUM_MAX];

/* ---------- STATIC FUNCTIONS PROTOTYPES ------------*/

static void uart_periph_driver_task(void *args);

static void uart_record_data_task(void *arg);

static void uart_apply_config(const uart_cfg_t *cfg);

static void uart_config_save_update(const uart_cfg_t *cfg);

/* ----------  GLOBAL FUNCTIONS SOURCES --------------*/

void uart_set_new_configure(uart_cfg_t *cfg)
{
    uart_config_save_update(cfg);
    uart_apply_config(cfg);

    static bool record_task_created = false;
    if (!record_task_created)
    {
        xTaskCreate(uart_record_data_task, "record_data_task", 3000, NULL, 4, NULL);
        record_task_created = true;
    }
}

/* --------- STATIC FUNCTIONS SOURCES ------------*/

static void uart_periph_driver_task(void *arg)
{
    int uart_num = (int)arg;

    ESP_LOGI(TAG, "TASK da UART%d iniciada!", uart_num);

    uart_event_t event;
    uint8_t *data = (uint8_t *)pvPortMalloc(2000);
    uint32_t written = 0;

    while (1)
    {
        if(xQueueReceive(uart_queue[uart_num], &event, portMAX_DELAY))
        {
            switch (event.type)
            {
                case UART_DATA:
                    int len = uart_read_bytes(uart_num, data, event.size, portMAX_DELAY);
                    utl_cbf_put_n(uart_circ_buffers[uart_num], data, len, &written);

                    break;

                case UART_BUFFER_FULL:
                    uart_flush_input(uart_num);
                    xQueueReset(uart_queue[uart_num]);
                
                default:
                    break;
            }
        }
    }
}

static void uart_record_data_task(void *arg)
{
    while (1)
    {
        for(uart_port_t uart_num = UART_NUM_0; uart_num < UART_NUM_MAX; uart_num++)
        {
            if(uart_installeds[uart_num])
            {
                uint32_t bytes_available = utl_cbf_bytes_available(uart_circ_buffers[uart_num]);
                if(bytes_available >= UART_MAX_PAYLOAD_LEN)
                {
                    sd_log_uart_t uart_msg;

                    uart_msg.LOG_PACKET_HEADER_INIT;
                    uart_msg.time_us = esp_timer_get_time();
                    uart_msg.log_type = SD_LOG_UART;
                    uart_msg.periph_num = uart_num;

                    uint32_t len = 0;
                    utl_cbf_get_n(uart_circ_buffers[uart_num], uart_msg.msg_data, UART_MAX_PAYLOAD_LEN, &len);
                    uart_msg.msg_len = (uint16_t)len;

                    sd_log_uart_data(&uart_msg);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void uart_config_save_update(const uart_cfg_t *cfg)
{
    sys_config_uart_t uart_cfg = {0};

    if (app_config_uart_load(&uart_cfg) != ESP_OK)
    {
        memset(&uart_cfg, 0, sizeof(uart_cfg));
    }

    uart_cfg.uarts[cfg->uart_num] = *cfg;

    uart_cfg.uart_cnt = 0;
    for (int i = 0; i < UART_NUM_MAX; i++)
    {
        if (uart_cfg.uarts[i].state)
            uart_cfg.uart_cnt++;
    }

    app_config_uart_save(&uart_cfg);
}

static void uart_apply_config(const uart_cfg_t *cfg)
{
    if (!cfg->state)
    {
        if (uart_installeds[cfg->uart_num])
        {
            uart_driver_delete(cfg->uart_num);
            vTaskDelete(uart_task_handlers[cfg->uart_num]);

            uart_task_handlers[cfg->uart_num] = NULL;
            uart_installeds[cfg->uart_num] = false;

            ESP_LOGI(TAG, "UART%d desabilitada", cfg->uart_num);
        }
        return;
    }

    if (uart_installeds[cfg->uart_num])
    {
        ESP_LOGW(TAG, "UART%d já instalada", cfg->uart_num);
        return;
    }

    uart_config_t uart_config =
    {
        .baud_rate = cfg->baudrate,
        .data_bits = UART_DATA_8_BITS,
        .stop_bits = UART_STOP_BITS_1,
        .parity    = UART_PARITY_DISABLE,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };

    uart_param_config(cfg->uart_num, &uart_config);
    uart_set_pin(cfg->uart_num, cfg->tx_pin, cfg->rx_pin,
                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    uart_driver_install(cfg->uart_num, 1024, 0, 20,
                        &uart_queue[cfg->uart_num], 0);

    xTaskCreate(uart_periph_driver_task, "uart_rx_task",
                4000, (void *)cfg->uart_num, 5,
                &uart_task_handlers[cfg->uart_num]);

    ESP_LOGI(TAG, "UART%d instalada! Tx: GPIO%d | Rx: GPIO%d a %dbps", cfg->uart_num, cfg->tx_pin, cfg->rx_pin, cfg->baudrate);

    uart_installeds[cfg->uart_num] = true;
}

bool uart_periph_driver_init(void)
{
    return true;
}

