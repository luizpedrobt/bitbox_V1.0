#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "driver/ledc.h"
#include "embled.h"
#include "hal/gpio_types.h"

#include "embled_app.h"

#define PWM_MODE       LEDC_LOW_SPEED_MODE
#define PWM_TIMER      LEDC_CHANNEL_0
#define PWM_RES        LEDC_TIMER_13_BIT
#define PWM_FREQ_HZ    1000

#define CHANNEL_A      LEDC_CHANNEL_0
#define CHANNEL_B      LEDC_CHANNEL_1

static const char *TAG = "EMBLED";

typedef enum port_pinout_e
{
    PORT_STATUS,
    PORT_OPER,
    PORT_MAX_LEDS,
}port_pinout_t;

static int embl_app_pins[PORT_MAX_LEDS] =
{
    GPIO_NUM_17, GPIO_NUM_18,
};

static int8_t profile_id;

static TimerHandle_t led_tmr_handler;
static StaticTimer_t led_tmr_buffer;

static TaskHandle_t app_led_handle;
static StaticTask_t app_led_buffer;

static StackType_t app_led_stack[128];

/* ---------- STATIC FUNCTIONS PROTOTYPES ------------*/

static void port_write_gpio(uint16_t pin, bool level);

static bool port_read_gpio(uint16_t pin);

static void embled_app_init(void);

static void embled_gpio_conf(void);

static void led_task(TimerHandle_t xTimer)
{
    embled_task(NULL);
}

/* ---------- STATIC FUNCTIONS SOURCES ------------*/
static void port_write_gpio(uint16_t pin, bool level)
{
    gpio_set_level(pin, (uint32_t)level);
}

static bool port_read_gpio(uint16_t pin)
{
    return(gpio_get_level(pin));
}

static void embled_app_init(void)
{
    embled_gpio_conf();

    static embled_callbacks_t led_callbacks =
    {
        .read_gpio = port_read_gpio,
        .write_gpio = port_write_gpio,
        .start_pwm = NULL,
        .stop_pwm = NULL,
    };

    embled_init(&led_callbacks);
    uint16_t duration[PORT_MAX_LEDS] = {100, 400};

    profile_id = embled_new_profile(PORT_MAX_LEDS, EMBLED_INFINITE, duration);

    led_tmr_handler = xTimerCreateStatic("LED_TMR", pdMS_TO_TICKS(EMBLED_CYCLE_TIME_MS), pdTRUE, NULL, led_task, &led_tmr_buffer);

    if(led_tmr_handler != NULL)
    {
        xTimerStart(led_tmr_handler, 0);
    }

    ESP_LOGI(TAG, "STATUS LED APP: %d", embled_set_mode(embl_app_pins[PORT_STATUS], EMBLED_DRIVER_MODE_DIGITAL, EMBLED_MODE_BLINK_SLOW, EMBLED_ACTIVE_HIGH, false));
    ESP_LOGI(TAG, "OPER LED APP:   %d", embled_set_mode(embl_app_pins[PORT_OPER]  , EMBLED_DRIVER_MODE_DIGITAL, EMBLED_MODE_BLINK_FAST, EMBLED_ACTIVE_HIGH, false));
}

static void embled_gpio_conf(void)
{
    gpio_config_t io_status_cfg =
    {
        .pin_bit_mask = 1ULL << embl_app_pins[PORT_STATUS],
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    gpio_config(&io_status_cfg);

    gpio_config_t io_oper_cfg =
    {
        .pin_bit_mask = 1ULL << embl_app_pins[PORT_OPER],
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    gpio_config(&io_oper_cfg);
}

/* ---------- MAIN APP ------------*/

void embled_app_main(void)
{
    ESP_LOGI(TAG, "Init LED app.");
    embled_app_init();
}