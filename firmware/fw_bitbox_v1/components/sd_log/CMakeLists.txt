idf_component_register(SRCS "sd_log.c"
                    REQUIRES uart_periph sdmmc_storage esp_timer
                    INCLUDE_DIRS "include")
