#pragma once

#define XMACRO_TOPIC_HANDLER_SUB_LISTS                           \
        X(CONFIG_REF, "topic/config", 12, handle_config_message) \
        X(OTA_REF   , "topic/ota"   ,  9, handle_ota_message   )  

typedef void (*topic_handler_fn)(const char *suffix, const char *payload);

typedef enum topic_reffs_e
{
    #define X(topic_ref, topic_name, topic_size, topic_handle_func) topic_ref,
        XMACRO_TOPIC_HANDLER_SUB_LISTS
    #undef X

    MAX_TOPICS,
}topic_reffs_t;

typedef struct topic_handler_s
{
    const char *topic;
    size_t topic_len;
    topic_handler_fn func;
}topic_handler_t;

void mqtt_main_app(void);