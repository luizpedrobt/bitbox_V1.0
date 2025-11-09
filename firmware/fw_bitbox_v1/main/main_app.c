/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "portmacro.h"

#include "driver/uart.h"
#include "hal/uart_types.h"

#include "utl_cbf.h"

#define RX_BUFFER_SIZE 100000
#define TX_BUFFER_SIZE RX_BUFFER_SIZE

const char *TAG = "MAIN_APP";

/*------- UART BUFFERS -------*/
UTL_CBF_DECLARE(uart_rx_buff_0, RX_BUFFER_SIZE);
UTL_CBF_DECLARE(uart_rx_buff_1, RX_BUFFER_SIZE);

/*------- UART CONFIGURATIONS -------*/ 
const uart_port_t uart_num = UART_NUM_0;
uart_config_t uart_cfg = 
{
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .stop_bits = UART_STOP_BITS_1,
    .parity = UART_PARITY_DISABLE,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .rx_flow_ctrl_thresh = 122,
};

/* ------- AUX VARS -------*/
int64_t t_start_buff_0 = 0;
int64_t t_start_buff_1 = 0;

int64_t t_end_buff_0 = 0;
int64_t t_end_buff_1 = 0;

int64_t t_buff_0 = 0;
int64_t t_buff_1 = 0;

bool buff_0_full = false;
bool buff_1_full = false;

static void app_init_periph(void)
{
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1, 100 *1024, 
                                        0, 0, 
                                        NULL, 0));
    
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_1, &uart_cfg));

    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1, 4, 
                                 5, UART_PIN_NO_CHANGE, 
                                 UART_PIN_NO_CHANGE));
}

static void app_rx_task(void *arg)
{
    t_start_buff_0 = esp_timer_get_time();
    static const char *RX_TAG = "RX_TASK";
    esp_log_level_set(RX_TAG, ESP_LOG_INFO);
    while (1)
    {
        static uint8_t c = 0;
        if(uart_read_bytes(UART_NUM_1, &c, 1, pdMS_TO_TICKS(10)))
        {
            if(utl_cbf_put(&uart_rx_buff_0, c) == UTL_CBF_FULL)
            {   
                if(!buff_0_full)
                {
                    t_end_buff_0 = esp_timer_get_time();
                    t_start_buff_1 = esp_timer_get_time();

                    t_buff_0 = t_end_buff_0 - t_start_buff_0;
                    ESP_LOGI(RX_TAG, "Buffer 0 FULL!! %"PRId64" us\n", t_buff_0);
                    ESP_LOGI(RX_TAG, "Starting fill buff 1!!\n");

                    buff_0_full = true;
                }
                
                if(utl_cbf_put(&uart_rx_buff_1, c) == UTL_CBF_FULL)
                {
                    if(!buff_1_full)
                    {
                        t_end_buff_1 = esp_timer_get_time();

                        t_buff_1 = t_end_buff_1 - t_start_buff_1;

                        ESP_LOGI(RX_TAG, "Buffer 1 FULL!! %"PRId64" us\n", t_buff_1);
                        ESP_LOGI(RX_TAG, "Stoping the receive!\n");
                        buff_1_full = true;
                    }
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "STARTING APP...");
    app_init_periph();
    xTaskCreate(app_rx_task, "uart_rx_task", 3072, NULL, 2, NULL);
}
