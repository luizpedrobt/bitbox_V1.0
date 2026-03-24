#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
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

#include "utl_crc16.h"
#include "utl_cobs.h"

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

static uint8_t uart_terminator_char[UART_NUM_MAX] = { 0 };

static portMUX_TYPE uart_mux[UART_NUM_MAX] = 
{
    portMUX_INITIALIZER_UNLOCKED,
    portMUX_INITIALIZER_UNLOCKED,
    portMUX_INITIALIZER_UNLOCKED
};

bool uart_installeds[UART_NUM_MAX] =
{
    false, false, false,
};

static TaskHandle_t uart_task_handlers[UART_NUM_MAX] = { NULL, NULL, NULL };
static TaskHandle_t uart_record_handlers[UART_NUM_MAX] = { NULL, NULL, NULL };

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

    xTaskCreate(uart_record_data_task, "record_data_task", 8192, (void *)cfg->uart_num, 1, &uart_record_handlers[cfg->uart_num]);
}

/* --------- STATIC FUNCTIONS SOURCES ------------*/

static void uart_periph_driver_task(void *arg)
{
    int uart_num = (int)arg;

    uart_event_t event;
    uint8_t *dtmp = (uint8_t *)pvPortMalloc(4092);
    assert(dtmp);

    while (1)
    {
        if (xQueueReceive(uart_queue[uart_num], &event, portMAX_DELAY))
        {
            if (event.type == UART_DATA)
            {
                uint32_t len = uart_read_bytes(uart_num, dtmp, event.size, portMAX_DELAY);

                if (len > 0)
                {
                    for (uint32_t i = 0; i < len; i++)
                    {
                        if (utl_cbf_put(uart_circ_buffers[uart_num], dtmp[i]) == UTL_CBF_FULL)
                        {
                            ESP_LOGW(TAG, "UART%d CBF FULL", uart_num);
                            break;
                        }
                    }

                    if (uart_record_handlers[uart_num] != NULL)
                    {
                        xTaskNotifyGive(uart_record_handlers[uart_num]);
                    }
                }
            }
            else if (event.type == UART_FIFO_OVF || event.type == UART_BUFFER_FULL)
            {
                uart_flush_input(uart_num);
                xQueueReset(uart_queue[uart_num]);
                ESP_LOGW(TAG, "UART%d Overflow", uart_num);
            }
        }

        if(!uart_installeds[uart_num])
        {
            ESP_LOGW(TAG, "Matando driver_task da UART%d!", uart_num);
            break;
        }
        
    }
    
    vPortFree(dtmp);

    uart_task_handlers[uart_num] = NULL;
    vTaskDelete(NULL);

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

    sd_log_msg_t *ctx = pvPortMalloc(sizeof(sd_log_msg_t));
    uint8_t *encoded_buf = pvPortMalloc(UART_MAX_PAYLOAD_LEN + (UART_MAX_PAYLOAD_LEN / 254) + 2);

    if (!ctx || !encoded_buf)
    {
        ESP_LOGE(TAG, "Memoria insuficiente UART%d", uart_num);
        vTaskDelete(NULL);
    }

    memset(ctx, 0, sizeof(sd_log_msg_t));
    uint8_t uart_byte;

    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        while (utl_cbf_bytes_available(uart_circ_buffers[uart_num]))
        {
            utl_cbf_get(uart_circ_buffers[uart_num], &uart_byte);

            if (uart_byte == uart_terminator_char[uart_num])
            {
                ESP_LOGI(TAG, "Terminador da UART%d: %d encontrado!", uart_num, uart_terminator_char[uart_num]);
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
                    ESP_LOGE(TAG, "Buffer Overflow UART%d. Resetando.", uart_num);
                    ctx->uart.payload_len = 0;
                }
            }
        }

        if(!uart_installeds[uart_num])
        {
            ESP_LOGW(TAG, "Matando record_task da UART%d!", uart_num);
            break;
        }
    }

    vPortFree(ctx);
    vPortFree(encoded_buf);

    uart_task_handlers[uart_num] = NULL;
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
        // .source_clk = UART_SCLK_APB, // JAMAIS USAR ESSA MERDA COM ESP_PM !!!!
        .source_clk = UART_SCLK_XTAL,
        .rx_flow_ctrl_thresh = 122,
    };

    uart_terminator_char[cfg->uart_num] = cfg->terminator_char;

    uart_param_config(cfg->uart_num, &uart_config);
    uart_set_pin(cfg->uart_num, cfg->tx_pin, cfg->rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    uart_driver_install(cfg->uart_num, 8192, 0, 20, &uart_queue[cfg->uart_num], ESP_INTR_FLAG_SHARED);
    gpio_set_pull_mode(cfg->rx_pin, GPIO_PULLUP_ONLY);
                        
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
