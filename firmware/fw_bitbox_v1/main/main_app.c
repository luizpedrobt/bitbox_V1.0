/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include "esp_log_level.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "embled_app.h"
#include "nvs_flash.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_pm.h"
#include "esp_system.h"

#include "sdmmc_storage.h"
#include "uart_periph.h"
#include "gpio_peripheral.h"
#include "wifi_conn.h"
#include "mqtt_app.h"
#include "sd_log.h"

#include "app_config.h"


static const char *TAG = "MAIN_APP";

/* ------- AUX VARS -------*/
bool sdmmc_initialized = false;
bool uart_periph_initialized = false;

/* ----------- STATIC FUNCTION DECLARATIONS --------------*/

static void app_init_periph(void);

/* ----------- STATIC FUNCTION SOURCE --------------*/

static void app_init_periph(void)
{
    esp_log_level_set("*", ESP_LOG_NONE);
    esp_log_level_set(TAG, ESP_LOG_INFO);

    embled_app_main();
    sdmmc_initialized = sdmmc_stor_init();

    if(wifi_conn_init())
    {
        ESP_LOGI(TAG, "Rede conectada! Partindo para o funcionamento online e offline!");
        mqtt_main_app();
    }

    else
    {
        ESP_LOGI(TAG, "Falha ao conectar na rede! Partindo para o funcionamento offline!");
    }

    uart_periph_initialized = uart_periph_driver_init();
    gpio_periph_main();

    esp_err_t ret = nvs_flash_init();
    ESP_ERROR_CHECK(ret);

    gpio_install_isr_service(ESP_INTR_FLAG_IRAM);

    app_config_main();
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
