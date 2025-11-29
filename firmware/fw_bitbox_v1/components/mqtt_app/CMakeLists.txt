idf_component_register(SRCS "mqtt_app.c"
                       REQUIRES mqtt main uart_periph json
                       EMBED_TXTFILES "certs/emqxsl-ca.crt"
                       INCLUDE_DIRS "include")
