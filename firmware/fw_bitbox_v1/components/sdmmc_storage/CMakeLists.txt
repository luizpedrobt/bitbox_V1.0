idf_component_register(SRCS "sdmmc_storage.c"
                       PRIV_REQUIRES fatfs esp_driver_sdmmc
                       INCLUDE_DIRS "include")
