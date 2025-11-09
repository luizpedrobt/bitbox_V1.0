idf_component_register(SRCS "main_app.c"
                       PRIV_REQUIRES utl_cbf esp_driver_uart esp_driver_gpio esp_timer
                       INCLUDE_DIRS ".")
