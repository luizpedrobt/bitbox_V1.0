idf_component_register(SRCS "embled_app.c" "embled.c"
                    REQUIRES esp_driver_gpio esp_driver_ledc
                    INCLUDE_DIRS "include")
