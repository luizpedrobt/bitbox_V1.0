#pragma once

#include "hal/uart_types.h"

#define RX_BUFFER_SIZE 1000
#define TX_BUFFER_SIZE RX_BUFFER_SIZE

typedef struct uart_cfg_s
{
    int uart_num;
    int rx_pin;
    int tx_pin;
    int baudrate;
}uart_cfg_t;

extern QueueHandle_t uart_queue[UART_NUM_MAX];

void uart_set_new_configure(uart_cfg_t *cfg);

bool uart_periph_driver_init(void);
