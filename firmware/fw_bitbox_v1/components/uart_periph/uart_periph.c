#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "uart_periph.h"

#include "portmacro.h"
#include "utl_cbf.h"

UTL_CBF_DECLARE(uart_rx_buff_0, RX_BUFFER_SIZE);
UTL_CBF_DECLARE(uart_rx_buff_1, RX_BUFFER_SIZE);

const char *TAG = "UART_PERIPH";

static QueueHandle_t uart_queue;

bool cbf_0_ready_to_fill = true;
bool cbf_1_ready_to_fill = true;

const uart_port_t uart_num = UART_NUM_0;
uart_config_t uart_cfg = 
{
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .stop_bits = UART_STOP_BITS_1,
    .parity = UART_PARITY_DISABLE,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .rx_flow_ctrl_thresh = 122,
};

static void process_uart_rx(uint8_t c)
{
    if(((utl_cbf_put(&uart_rx_buff_0, c) == UTL_CBF_FULL)) && cbf_0_ready_to_fill)
        utl_cbf_put(&uart_rx_buff_1, c);
}

void uart_driver_init(void);
{   
    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_cfg));

    ESP_ERROR_CHECK(uart_set_pin(uart_num, 4, 
                                 5, UART_PIN_NO_CHANGE, 
                                 UART_PIN_NO_CHANGE));

    ESP_ERROR_CHECK(uart_driver_install(uart_num, 
                                    100 *1024, 
                                    0, 
                                    20, 
                                    &uart_queue, 
                                    0));
}

void uart_driver_task(void *arg)
{
    uart_event_t event;
    static uint8_t c = 0;

    while (1)
    {
        if(xQueueReceive(uart_queue, &event, portMAX_DELAY))
        {
            switch (event.type)
            {
                case UART_DATA:
                    uart_read_bytes(UART_NUM_1, &c, event.size, portMAX_DELAY);
                    process_uart_rx(c);
                    break;

                case UART_BUFFER_FULL:
                    uart_flush_input(uart_num);
                    xQueueReset(uart_queue);
                
                default:
                    break;
            }
        }
    }
}