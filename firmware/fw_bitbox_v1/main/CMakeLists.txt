idf_component_register(SRCS "app_config.c" "app_tasks.c" "main_app.c"
                       PRIV_REQUIRES sd_log cjson utl_cbf utl_io esp_driver_gpio esp_timer sdmmc_storage uart_periph nvs_flash esp_http_client esp_https_ota wifi_conn embled mqtt_app
                       INCLUDE_DIRS "include")
