idf_component_register(SRCS "uart_periph.c"
                       PRIV_REQUIRES sd_log esp_driver_uart utl_cbf main sdmmc_storage cjson esp_timer
                       INCLUDE_DIRS "include")
