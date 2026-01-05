idf_component_register(SRCS "mqtt_app.c"
                       REQUIRES mqtt main uart_periph gpio_periph cjson 
                       EMBED_TXTFILES "certs/emqxsl-ca.crt"
                       INCLUDE_DIRS "include")
