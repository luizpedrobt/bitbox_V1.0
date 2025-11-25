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
#include "embled_app.h"
#include "nvs_flash.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"

#include "sdmmc_storage.h"
#include "uart_periph.h"
#include "wifi_conn.h"
#include "mqtt_app.h"

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

    esp_err_t ret = nvs_flash_init();
    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_conn_init();
    embled_app_main();
    mqtt_main_app();
}

/* --------- FUNCTION SOURCE ------------------------*/

void app_main(void)
{
    ESP_LOGI(TAG, "STARTING APP...");
    app_init_periph();
    printf("--- DIAGNOSTICO DE MEMORIA ---\n");
    printf("Total Livre: %lu bytes\n", esp_get_free_heap_size());
    printf("Maior Bloco Livre: %u bytes\n", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    printf("PSRAM Livre      : %d bytes (%d KB)\n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM), heap_caps_get_free_size(MALLOC_CAP_SPIRAM)/1024);
    printf("------------------------------\n");
}

bool main_app_get_uart_init_flag(void)
{
    return uart_periph_initialized;
}

bool main_app_get_sdmmc_init_flag(void)
{
    return sdmmc_initialized;
}
