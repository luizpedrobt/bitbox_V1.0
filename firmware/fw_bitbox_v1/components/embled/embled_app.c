#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "embled.h"

#include "embled_app.h"

#define PWM_MODE       LEDC_LOW_SPEED_MODE
#define PWM_TIMER      LEDC_CHANNEL_0
#define PWM_RES        LEDC_TIMER_13_BIT
#define PWM_FREQ_HZ    1000

#define CHANNEL_A      LEDC_CHANNEL_0
#define CHANNEL_B      LEDC_CHANNEL_1

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

static void led_task(void *arg)
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
    static embled_callbacks_t led_callbacks =
    {
        .read_gpio = port_read_gpio,
        .write_gpio = port_write_gpio,
        .start_pwm = NULL,
        .stop_pwm = NULL,
    };

    embled_init(&led_callbacks);
    uint16_t duration[PORT_MAX_LEDS] = {100, 404};

    profile_id = embled_new_profile(PORT_MAX_LEDS, EMBLED_INFINITE, duration);

    led_tmr_handler = xTimerCreateStatic("LED_TMR", pdMS_TO_TICKS(EMBLED_CYCLE_TIME_MS), pdTRUE, NULL, led_task, &led_tmr_buffer);

    if(led_tmr_handler != NULL)
    {
        xTimerStart(led_tmr_handler, 0);
    }
}

/* ---------- MAIN APP ------------*/

void embled_app_main(void)
{
    embled_app_init();
}