idf_component_register(SRCS "app_config.c" "app_tasks.c" "main_app.c"
                       PRIV_REQUIRES utl_cbf utl_io esp_driver_uart esp_driver_gpio esp_timer fatfs esp_driver_sdmmc sdmmc_storage
                       INCLUDE_DIRS "include")
