#include <stdint.h>
#include <stdlib.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "hal/uart_types.h"
#include "nvs.h"
#include "uart_periph.h"
#include "gpio_peripheral.h"
#include "nvs_flash.h"
#include "esp_err.h"
#include "portmacro.h"

#include "app_config.h"

static const char *TAG = "APP_CONFIG";

static const int gpio_pins[GPIO_BOARD_MAX] = { 1 , 2, 3, 4, 5, 33, 34, 35, 37, 38 };

#define X(IDX, PERIPH_CFG, ST_PERIPH, CFG_VAR, NAMESPACE, FUNC_PROT) const char *CFG_VAR##_namespace = NAMESPACE;
    XMACRO_SYS_CONFIG
#undef X

esp_err_t app_config_uart_load(sys_config_uart_t *uart_cfg)
{
    nvs_handle_t nvs_handler;
    esp_err_t err;

    size_t size = sizeof(sys_config_uart_t);

    err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &nvs_handler);
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "Erro ao abrir configuração anterior!");
        return err;
    }

    err = nvs_get_blob(nvs_handler, uart_cfg_namespace, uart_cfg, &size);
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "Erro ao ler configuração da UART!");
        return err;
    }
 
    nvs_close(nvs_handler);
    return err;
}

esp_err_t app_config_gpio_load(sys_config_gpio_t *gpio_cfg)
{
    nvs_handle_t nvs_handler;
    esp_err_t err;

    size_t size = sizeof(sys_config_gpio_t);

    err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &nvs_handler);
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "Erro ao abrir configuração anterior!");
        return err;
    }

    err = nvs_get_blob(nvs_handler, gpio_cfg_namespace, gpio_cfg, &size);
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "Erro ao ler configuração do GPIO!");
        return err;
    }
 
    nvs_close(nvs_handler);
    return err;
}


esp_err_t app_config_uart_save(const sys_config_uart_t *uart_cfg)
{
    nvs_handle_t nvs_handler;
    esp_err_t err;

    size_t size = sizeof(sys_config_uart_t);

    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvs_handler);
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "Erro ao abrir nvs para salvar configuração!");
        return err;
    }

    err = nvs_set_blob(nvs_handler, uart_cfg_namespace, uart_cfg, size);
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "Erro ao salvar configurações de UART!");
        return err;
    }

    err = nvs_commit(nvs_handler);
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "Erro ao commitar!");
        return err;
    }

    nvs_close(nvs_handler);
    return err;
}

esp_err_t app_config_gpio_save(const sys_config_gpio_t *gpio_cfg)
{
    nvs_handle_t nvs_handler;
    esp_err_t err;

    size_t size = sizeof(sys_config_gpio_t);

    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvs_handler);
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "Erro ao abrir nvs para salvar configuração!");
        return err;
    }

    err = nvs_set_blob(nvs_handler, gpio_cfg_namespace, gpio_cfg, size);
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "Erro ao salvar configurações de UART!");
        return err;
    }

    err = nvs_commit(nvs_handler);
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "Erro ao commitar!");
        return err;
    }

    nvs_close(nvs_handler);
    return err;
}

void app_config_main(void)
{
    ESP_LOGI(TAG, "Carregando configurações salvas...");

    sys_config_uart_t uart_cfg = {0};
    sys_config_gpio_t gpio_cfg = {0};

    /* ---------- UART ---------- */
    app_config_uart_load(&uart_cfg);

    ESP_LOGI(TAG, "Configurações de UART carregadas: %d UART(s)", uart_cfg.uart_cnt);

    for (uint8_t i = 0; i < UART_NUM_MAX; i++)
    {
        if (uart_cfg.uarts[i].state)
        {
            ESP_LOGI(TAG, "Inicializando UART%d", uart_cfg.uarts[i].uart_num);
            uart_set_new_configure(&uart_cfg.uarts[i]);
        }
    }

    /* ---------- GPIO ---------- */
    app_config_gpio_load(&gpio_cfg);

    ESP_LOGI(TAG, "Configurações de GPIO carregadas: %d GPIO(s)", gpio_cfg.gpio_cnt);

    for (uint8_t i = 0; i < GPIO_BOARD_MAX; i++)
    {
        if (gpio_cfg.gpios[i].state)
        {
            ESP_LOGI(TAG, "Inicializando GPIO%d", gpio_pins[gpio_cfg.gpios[i].gpio_num]);
            gpio_set_new_configure(&gpio_cfg.gpios[i]);
        }
    }

    ESP_LOGI(TAG, "Configuracoes aplicadas com sucesso!");
}


