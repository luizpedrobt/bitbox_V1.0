#pragma once

#include "driver/gpio.h"
#include "hal/gpio_types.h"

typedef enum gpio_available_ports_s
{
    GPIO_BOARD_1 = 0,
    GPIO_BOARD_2,
    GPIO_BOARD_3,
    GPIO_BOARD_4,
    GPIO_BOARD_5,

    GPIO_BOARD_33,
    GPIO_BOARD_34,
    GPIO_BOARD_35,
    GPIO_BOARD_36,
    GPIO_BOARD_37,

    GPIO_BOARD_MAX,
} gpio_available_ports_t;

typedef struct gpio_cfg_s
{
    gpio_available_ports_t gpio_num;
    gpio_mode_t mode;               
    gpio_pullup_t pull_up_en;       
    gpio_pulldown_t pull_down_en;   
    gpio_int_type_t intr_type;   
    bool state;
} gpio_cfg_t;

void gpio_set_new_configure(gpio_cfg_t *cfg);

void gpio_periph_main(void);
