#pragma once

#define PACKED __attribute__((packed))

#define LOG_PACKET_HEADER_INIT header = 0xdeadbeef

#define UART_MAX_PAYLOAD_LEN 1024

typedef enum sd_log_type_e
{
    SD_LOG_UART = 1,
    SD_LOG_GPIO,

    SD_LOG_NO_SUPPORTED,
} sd_log_type_t;

typedef enum sd_log_uart_num_e
{
    SD_LOG_UART_0 = 0,
    SD_LOG_UART_1,
    SD_LOG_UART_2,

    SD_LOG_UART_NONE,
} sd_log_uart_num_t;

typedef enum sd_log_gpio_num_e
{
    // left conn
    SD_LOG_GPIO_1 = 1,
    SD_LOG_GPIO_2,
    SD_LOG_GPIO_3,
    SD_LOG_GPIO_4,
    SD_LOG_GPIO_5,

    // right conn
    SD_LOG_GPIO_33 = 33,
    SD_LOG_GPIO_34,
    SD_LOG_GPIO_35,
    SD_LOG_GPIO_36,
    SD_LOG_GPIO_37,

    SD_LOG_GPIO_NONE,
} sd_log_gpio_num_t;

typedef struct PACKED sd_log_uart_s
{
    uint32_t header;
    uint64_t time_us;
    uint8_t log_type;
    uint8_t periph_num;
    uint16_t msg_len;
    uint8_t msg_data[UART_MAX_PAYLOAD_LEN];
} sd_log_uart_t;

typedef struct PACKED sd_log_gpio_s
{
    uint32_t header;
    uint64_t time_us;
    uint8_t type;
    uint8_t periph_num;
    uint8_t edge;
    uint8_t level;
} sd_log_gpio_t;

typedef struct PACKED sd_log_msg_s
{
    sd_log_type_t type;
    union
    {
        sd_log_uart_t uart;
        sd_log_gpio_t gpio;
    };
} sd_log_msg_t;


bool sd_log_uart_data(sd_log_uart_t *msg);

bool sd_log_gpio_data(sd_log_gpio_t *msg);

void sd_log_main(void);
