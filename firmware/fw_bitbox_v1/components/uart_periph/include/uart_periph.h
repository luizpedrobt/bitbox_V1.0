#pragma once

#include "hal/uart_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define RX_BUFFER_SIZE 1000
#define TX_BUFFER_SIZE RX_BUFFER_SIZE

typedef struct uart_cfg_s
{
    int uart_num;
    int rx_pin;
    int tx_pin;
    int baudrate;
    bool state; // false = disable | true = enable
    uint8_t terminator_char;
} uart_cfg_t;

extern QueueHandle_t uart_queue[UART_NUM_MAX];

void uart_set_new_configure(uart_cfg_t *cfg);

int uart_periph_write_data(int uart_num, const void *src, size_t size);


bool uart_periph_driver_init(void);
