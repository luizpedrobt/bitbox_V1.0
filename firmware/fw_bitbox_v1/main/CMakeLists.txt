idf_component_register(SRCS "app_config.c" "main_app.c"
                       REQUIRES sd_log esp_pm cjson utl_cbf utl_io gpio_periph esp_driver_gpio esp_timer sdmmc_storage uart_periph nvs_flash esp_http_client esp_https_ota wifi_conn embled mqtt_app
                       INCLUDE_DIRS "include")
