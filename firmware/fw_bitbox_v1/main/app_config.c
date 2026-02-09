#include <stdint.h>
#include <stdlib.h>
#include "esp_attr.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "hal/uart_types.h"
#include "nvs.h"
#include "uart_periph.h"
#include "gpio_peripheral.h"
#include "nvs_flash.h"
#include "esp_err.h"
#include "portmacro.h"

#include "app_config.h"

#define GPIO_BTN_RESET GPIO_NUM_0

#define RESET_HOLD_TIME_MS 3000

static const char *TAG = "APP_CONFIG";

const int gpio_pins[GPIO_BOARD_MAX] = { 1 , 2, 3, 4, 5, 33, 34, 35, 36, 37 };

#define X(IDX, PERIPH_CFG, ST_PERIPH, CFG_VAR, NAMESPACE, FUNC_PROT) const char *CFG_VAR##_namespace = NAMESPACE;
    XMACRO_SYS_CONFIG
#undef X

bool online_mode = false;

static TaskHandle_t reset_task_handle = NULL;

static void IRAM_ATTR reset_button_isr(void *arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if(reset_task_handle != NULL)
    {
        xTaskNotifyFromISR(reset_task_handle, 1, eSetBits, &xHigherPriorityTaskWoken);
    }

    else
    {
        portYIELD_FROM_ISR();
    }
}

static void reset_button_task(void *arg)
{
    uint32_t notify_value;
    
    while(1)
    {
        xTaskNotifyWait(0, ULONG_MAX, &notify_value, portMAX_DELAY);

        ESP_LOGW(TAG, "Botão de reset pressionado. Segure por 3 s ...");

        bool reset_confirmed = true;

        for(uint32_t reset_counter = 0; reset_counter < (RESET_HOLD_TIME_MS / 100); reset_counter++)
        {
            vTaskDelay(pdMS_TO_TICKS(100));

            if(gpio_get_level(GPIO_BTN_RESET) == 1)
            {
                ESP_LOGI(TAG, "Reset cancelado.");
                reset_confirmed = false;
                break;
            }
        }

        if(reset_confirmed)
        {
            app_config_erase_all();
            ESP_LOGI(TAG, "Reiniciado...");
            vTaskDelay(pdMS_TO_TICKS(1000));
            esp_restart();
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void app_config_init_reset_btn(void)
{
    gpio_config_t gpio_cfg =
    {
        .pin_bit_mask = (1ULL << GPIO_BTN_RESET),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };

    gpio_config(&gpio_cfg);

    xTaskCreate(reset_button_task, "reset_button_task", 4096, NULL, 10, &reset_task_handle);

    gpio_isr_handler_add(GPIO_BTN_RESET, reset_button_isr, NULL);
}

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

esp_err_t app_config_netw_load(sys_config_netw_t *netw_cfg)
{
    nvs_handle_t nvs_handler;
    esp_err_t err;

    size_t size = sizeof(sys_config_netw_t);

    err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &nvs_handler);
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "Erro ao abri configuração de rede anterior!");
        return err;
    }

    err = nvs_get_blob(nvs_handler, netw_cfg_namespace, netw_cfg, &size);
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "Erro ao setar configuração de rede anterior!");
        return err;
    }

    nvs_close(nvs_handler);
    return err;
}

esp_err_t app_config_netw_save(const sys_config_netw_t *netw_cfg)
{
    nvs_handle_t nvs_handler;
    esp_err_t err;

    size_t size = sizeof(sys_config_netw_t);

    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvs_handler);
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "Erro ao abrir nvs para salvar configuração!");
        return err;
    }

    err = nvs_set_blob(nvs_handler, netw_cfg_namespace, netw_cfg, size);
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "Erro ao salvar configurações de rede!");
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

esp_err_t app_config_erase_all(void)
{
    nvs_handle_t nvs_handler;
    esp_err_t err;

    ESP_LOGW(TAG, "Iniciando processo de limpeza de configuração!");

    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvs_handler);
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "Erro ao abrir chaves do NVS!");
        nvs_close(nvs_handler);
        return err;
    }

    err = nvs_erase_all(nvs_handler);
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "Erro ao apagar chaves do NVS!");
        nvs_close(nvs_handler);
        return err;
    }

    err = nvs_commit(nvs_handler);
    nvs_close(nvs_handler);

    ESP_LOGI(TAG, "Configuracoes apagadas com sucesso!");
    return err;
}

bool app_config_get_online_mode(void)
{
    return online_mode;
}

void app_config_main(void)
{
    esp_log_level_set(TAG, ESP_LOG_INFO);
    ESP_LOGI(TAG, "Carregando configurações salvas...");

    sys_config_uart_t uart_cfg = {0};
    sys_config_gpio_t gpio_cfg = {0};

    bool pin_initialized[40] = {false}; 

    /* ---------- UART ---------- */
    app_config_uart_load(&uart_cfg);
    ESP_LOGI(TAG, "UARTs carregadas: %d", uart_cfg.uart_cnt);

    for (uint8_t i = 0; i < UART_NUM_MAX; i++)
    {
        if (uart_cfg.uarts[i].state)
        {
            if (uart_cfg.uarts[i].tx_pin < 0 || uart_cfg.uarts[i].rx_pin < 0) 
                continue;

            ESP_LOGI(TAG, "Inicializando UART%d", uart_cfg.uarts[i].uart_num);
            uart_set_new_configure(&uart_cfg.uarts[i]);
            

            if(uart_cfg.uarts[i].tx_pin < 40) pin_initialized[uart_cfg.uarts[i].tx_pin] = true;
            if(uart_cfg.uarts[i].rx_pin < 40) pin_initialized[uart_cfg.uarts[i].rx_pin] = true;
        }
    }

    /* ---------- GPIO ---------- */
    app_config_gpio_load(&gpio_cfg);
    ESP_LOGI(TAG, "GPIOs carregados: %d", gpio_cfg.gpio_cnt);

    for (uint8_t i = 0; i < GPIO_BOARD_MAX; i++)
    {
        if (gpio_cfg.gpios[i].state)
        {
            int pino_fisico = gpio_pins[gpio_cfg.gpios[i].gpio_num];

            if (pino_fisico < 40 && pin_initialized[pino_fisico])
            {
                ESP_LOGW(TAG, "Ignorando GPIO%d duplicado (Configuração suja detectada)", pino_fisico);
                continue;
            }

            ESP_LOGI(TAG, "Inicializando GPIO%d", pino_fisico);
            gpio_set_new_configure(&gpio_cfg.gpios[i]);
            
            if (pino_fisico < 40) pin_initialized[pino_fisico] = true;
        }
    }
    
    ESP_LOGI(TAG, "Configuracoes aplicadas com sucesso!");
    app_config_init_reset_btn();
}


