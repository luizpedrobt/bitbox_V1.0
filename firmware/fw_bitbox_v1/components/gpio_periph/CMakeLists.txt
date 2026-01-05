idf_component_register(SRCS "gpio_peripheral.c"
                    REQUIRES sd_log esp_driver_gpio main uart_periph
                    INCLUDE_DIRS "include")
