idf_component_register(SRCS "uart_periph.c"
                       PRIV_REQUIRES esp_driver_uart utl_cbf main sdmmc_storage json
                       INCLUDE_DIRS "include")
