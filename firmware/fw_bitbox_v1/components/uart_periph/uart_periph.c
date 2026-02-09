#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "esp_system.h"
#include "freertos/idf_additions.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/uart.h"

#include "esp_timer.h"

#include "hal/uart_types.h"
#include "mqtt_app.h"
#include "uart_periph.h"
#include "gpio_peripheral.h"
#include "sd_log.h"

#include "portmacro.h"
#include "utl_cbf.h"

#include "app_config.h"

#define UART_PARSE_BUDGET 128   

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

bool uart_installeds[UART_NUM_MAX] =
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

    xTaskCreate(uart_record_data_task, "record_data_task", 8192, (void *)cfg->uart_num, 1, NULL);
}

/* --------- STATIC FUNCTIONS SOURCES ------------*/

static void uart_periph_driver_task(void *arg)
{
    int uart_num = (int)arg;
    ESP_LOGI(TAG, "TASK da UART%d iniciada!", uart_num);

    uart_event_t event;
    uint8_t data[2048];
    uint32_t written = 0;

    while (1)
    {
        if(xQueueReceive(uart_queue[uart_num], &event, pdMS_TO_TICKS(10)))
        {
            switch (event.type)
            {
                case UART_DATA:
                    int len = uart_read_bytes(uart_num, data, event.size, pdMS_TO_TICKS(100));
                    if(len > 0)
                    {
                        utl_cbf_put_n(uart_circ_buffers[uart_num], data, len, &written);

                        ESP_LOGI(TAG, "TASK UART%d: <- %d bytes armazenados no buffer circular!", uart_num, event.size);
                    }
                    
                    break;

                case UART_FIFO_OVF:
                case UART_BUFFER_FULL:
                    uart_flush_input(uart_num);
                    xQueueReset(uart_queue[uart_num]);
                    ESP_LOGW(TAG, "UART%d Buffer Full/Overflow", uart_num);
                
                    break;
                
                default:
                    break;
            }
        }
    }
}

static void process_uart_packet(uart_port_t uart_num, sd_log_msg_t *ctx)
{
    if (ctx->uart.payload_len > 0)
    {
        ctx->log_header.header     = LOG_PACKET_HEADER_INIT;

        ctx->log_header.time_us    = esp_timer_get_time(); 
        ctx->log_header.log_type   = SD_LOG_UART;
        ctx->log_header.periph_num = uart_num;

        sd_log_data(ctx);
        mqtt_publish_msg(ctx);
        
        ctx->uart.payload_len = 0;
    }
}

static void uart_record_data_task(void *arg)
{
    uart_port_t uart_num = (uart_port_t)arg;

    sd_log_msg_t *ctx = (sd_log_msg_t *)malloc(sizeof(sd_log_msg_t));
    if (ctx == NULL) 
    {
        ESP_LOGE(TAG, "Falha de memoria na UART%d", uart_num);
        vTaskDelete(NULL);
    }

    memset(ctx, 0, sizeof(sd_log_msg_t));

    static uint8_t uart_byte;

    while (1)
    {
        int processed = 0;

        while (utl_cbf_bytes_available(uart_circ_buffers[uart_num]) && processed < UART_PARSE_BUDGET)
        {
            utl_cbf_get(uart_circ_buffers[uart_num], &uart_byte);
            processed++;

            if (uart_byte == 0x00)
            {
                ESP_LOGI(TAG, "\\0 detectado. %d bytes. UART%d", ctx->uart.payload_len, uart_num);

                process_uart_packet(uart_num, ctx);
            }

            else
            {
                if (ctx->uart.payload_len < UART_MAX_PAYLOAD_LEN)
                {
                    ctx->uart.payload[ctx->uart.payload_len++] = uart_byte;
                }

                else
                {
                    ESP_LOGW(TAG, "Buffer cheio. Resetando frame.");
                    ctx->uart.payload_len = 0;
                }
            }
        }
        taskYIELD();
    }
    
    free(ctx);
    vTaskDelete(NULL);
}

static void uart_config_save_update(const uart_cfg_t *cfg)
{
    sys_config_uart_t uart_cfg = { 0 };

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

            if (uart_task_handlers[cfg->uart_num] != NULL)
            {
                vTaskDelete(uart_task_handlers[cfg->uart_num]);
                uart_task_handlers[cfg->uart_num] = NULL;
            }

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
    uart_set_pin(cfg->uart_num, cfg->tx_pin, cfg->rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(cfg->uart_num, 8192, 0, 20, &uart_queue[cfg->uart_num], 0);
                        
    xTaskCreate(uart_periph_driver_task, "uart_rx_task",8192, (void *)cfg->uart_num, 10, &uart_task_handlers[cfg->uart_num]);

    ESP_LOGI(TAG, "UART%d instalada! Tx: GPIO%d | Rx: GPIO%d a %dbps", cfg->uart_num, cfg->tx_pin, cfg->rx_pin, cfg->baudrate);
    uart_installeds[cfg->uart_num] = true;
}

int uart_periph_write_data(int uart_num, const void *src, size_t size)
{   
    return(uart_write_bytes(uart_num, src, size));
}

bool uart_periph_driver_init(void)
{
    esp_log_level_set(TAG, ESP_LOG_INFO);
    return true;
}
