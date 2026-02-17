idf_component_register(SRCS "uart_periph.c"
                       PRIV_REQUIRES sd_log esp_driver_uart utl_cbf main esp_timer gpio_periph mqtt_app utl_crc16 utl_cobs
                       INCLUDE_DIRS "include")
