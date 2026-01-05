#include <stdio.h>
#include <stdint.h>
#include "gpio_peripheral.h"

#include "esp_attr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "hal/gpio_types.h"

#include "sd_log.h"
#include "uart_periph.h"
#include "app_config.h"

#define GPIO_EVT_QUEUE_LEN 64

#define GPIO_DEBOUNCE_US 50000  

static uint64_t last_evt_time[GPIO_BOARD_MAX] = {0};
static uint8_t  last_level[GPIO_BOARD_MAX]   = {0xFF};

static QueueHandle_t gpio_evt_queue;

bool gpio_installeds[GPIO_BOARD_MAX] = { false, false, false, false, false, false, false, false, false, false };
static const int gpio_pins[GPIO_BOARD_MAX] = { 1 , 2, 3, 4, 5, 33, 34, 35, 37, 38 };

static const char *TAG = "GPIO_PERIPH";

static bool first_run = false;

typedef struct gpio_evt_s
{
    uint8_t gpio;
    uint8_t level;
    uint64_t time_us;
} gpio_evt_t;

/* ---------- STATIC FUNCTIONS PROTOTYPES ------------*/

static void gpio_apply_config(const gpio_cfg_t *cfg);

static void gpio_config_save_update(const gpio_cfg_t *cfg);

static void gpio_isr_handler(void *args);

static void gpio_log_task(void *args);

/* ----------  GLOBAL FUNCTIONS SOURCES --------------*/

void gpio_set_new_configure(gpio_cfg_t *cfg)
{
    gpio_config_save_update(cfg);
    gpio_apply_config(cfg);

    static bool record_task_created = false;
    if (!record_task_created)
    {
        xTaskCreate(gpio_log_task, "gpio_log_task", 4095, NULL, 5, NULL);
        record_task_created = true;
    }
}

void gpio_periph_main(void)
{
    gpio_evt_queue = xQueueCreate( GPIO_EVT_QUEUE_LEN, sizeof(gpio_evt_t));
}

/* --------- STATIC FUNCTIONS SOURCES ------------*/

static void gpio_apply_config(const gpio_cfg_t *cfg)
{
    if(!cfg->state)
    {
        if(gpio_installeds[cfg->gpio_num])
        {
            gpio_isr_handler_remove(gpio_pins[cfg->gpio_num]);
            gpio_reset_pin(gpio_pins[cfg->gpio_num]);

            gpio_installeds[cfg->gpio_num] = false;
            ESP_LOGI(TAG, "GPIO%d desabilitado!", gpio_pins[cfg->gpio_num]);
            return;
        }
    }

    if(gpio_installeds[cfg->gpio_num])
    {
        ESP_LOGI(TAG, "GPIO%d já instalado!", gpio_pins[cfg->gpio_num]);
        return;
    }

    gpio_config_t gpio_cfg =    
    {
        .intr_type = cfg->intr_type,
        .mode = cfg->mode,
        .pin_bit_mask = 1ULL << gpio_pins[cfg->gpio_num],
        .pull_down_en = cfg->pull_down_en,
        .pull_up_en = cfg->pull_up_en,
    };

    if(!first_run)
    {
        gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
        first_run = true;
    }

    gpio_config(&gpio_cfg);

    gpio_isr_handler_add(gpio_pins[cfg->gpio_num], gpio_isr_handler, (void*)gpio_pins[cfg->gpio_num]);
    
    gpio_installeds[cfg->gpio_num] = true;

    ESP_LOGI(TAG, "GPIO%d configurado!", gpio_pins[cfg->gpio_num]);
}

static void gpio_config_save_update(const gpio_cfg_t *cfg)
{
    sys_config_gpio_t gpio_cfg = { 0 };

    if(app_config_gpio_load(&gpio_cfg) != ESP_OK)
    {
        memset(&gpio_cfg, 0, sizeof(gpio_cfg));
    }

    gpio_cfg.gpios[cfg->gpio_num] = *cfg;

    gpio_cfg.gpio_cnt = 0;
    for(int gpio_idx = 0; gpio_idx < GPIO_BOARD_MAX; gpio_idx++)
    {
        if(gpio_cfg.gpios[gpio_idx].state)
        {
            gpio_cfg.gpio_cnt++;
        }
    }

    app_config_gpio_save(&gpio_cfg);
}

static void IRAM_ATTR gpio_isr_handler(void *args)
{
    uint32_t gpio_num = (uint32_t)args; 

    gpio_evt_t evt; 
    evt.gpio = gpio_num; 
    evt.level = gpio_get_level(gpio_num); 
    evt.time_us = esp_timer_get_time();

    xQueueSendFromISR(gpio_evt_queue, &evt, NULL);
}

static void gpio_log_task(void *args)
{
    gpio_evt_t evt;

    while (1)
    {
        if (xQueueReceive(gpio_evt_queue, &evt, portMAX_DELAY))
        {
            uint32_t gpio = evt.gpio;
            uint64_t now  = evt.time_us;

            if (gpio >= GPIO_BOARD_MAX) {
                continue;
            }

            if ((now - last_evt_time[gpio]) < GPIO_DEBOUNCE_US) {
                continue;  
            }

            if (last_level[gpio] == evt.level) {
                continue;
            }

            last_evt_time[gpio] = now;
            last_level[gpio]    = evt.level;

            ESP_LOGI(TAG, "GPIO%d | level=%d | t=%lld us", gpio, evt.level, now);

            sd_log_gpio_t gpio_msg = {0};

            gpio_msg.LOG_PACKET_HEADER_INIT;
            gpio_msg.time_us    = now;
            gpio_msg.type       = SD_LOG_GPIO;
            gpio_msg.periph_num = gpio;
            gpio_msg.edge       = evt.level;
            gpio_msg.level      = evt.level;

            sd_log_gpio_data(&gpio_msg);
        }
    }
}
