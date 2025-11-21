/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "sdmmc_storage.h"
#include "uart_periph.h"
#include "wifi_conn.h"

static const char *TAG = "MAIN_APP";

/* ------- AUX VARS -------*/
bool sdmmc_initialized = false;
bool uart_periph_initialized = false;

/* ----------- STATIC FUNCTION DECLARATIONS --------------*/

static void app_init_periph(void);

/* ----------- STATIC FUNCTION SOURCE --------------*/

static void app_init_periph(void)
{
    sdmmc_initialized = sdmmc_stor_init();
    uart_periph_initialized = uart_periph_driver_init();
    wifi_conn_init();
}

/* --------- FUNCTION SOURCE ------------------------*/

void app_main(void)
{
    ESP_LOGI(TAG, "STARTING APP...");
    app_init_periph();
}

bool main_app_get_uart_init_flag(void)
{
    return uart_periph_initialized;
}

bool main_app_get_sdmmc_init_flag(void)
{
    return sdmmc_initialized;
}
