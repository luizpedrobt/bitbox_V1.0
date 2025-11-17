#include <stdio.h>
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "uart_periph.h"
#include "sdmmc_storage.h"

#include "portmacro.h"
#include "utl_cbf.h"

#define STRINGFY(x) #x

UTL_CBF_DECLARE(uart_rx_buff_0, RX_BUFFER_SIZE);
UTL_CBF_DECLARE(uart_rx_buff_1, RX_BUFFER_SIZE);

static const char *TAG = "UART_PERIPH";

static QueueHandle_t uart_queue;

typedef enum uart_buff_e
{
    UART_BUFFER_0,
    UART_BUFFER_1,
}uart_buff_t;

static uart_buff_t active_buffer = UART_BUFFER_0;

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
/* ---------- STATIC FUNCTIONS PROTOTYPES ------------*/

static void uart_periph_process_uart_rx(uint8_t c); // uart "cbk" 

static size_t uart_periph_get_buf_data(uint8_t *p_buff, size_t size);

static bool uart_periph_record_data_SD(uint8_t *p_buff, size_t size, const char *file_name);

/* ----------  GLOBAL FUNCTIONS SOURCES --------------*/

void uart_periph_driver_init(void)
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

void uart_periph_driver_task(void *arg)
{
    uart_event_t event;
    uint8_t data[256] = {0};

    while (1)
    {
        if(xQueueReceive(uart_queue, &event, portMAX_DELAY))
        {
            switch (event.type)
            {
                case UART_DATA:
                    int len = uart_read_bytes(uart_num, data, event.size, portMAX_DELAY);
                    for(int idx = 0; idx < len; idx++)
                        process_uart_rx(data[idx]);

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

/* --------- STATIC FUNCTIONS SOURCES ------------*/

static size_t uart_periph_get_buf_data(uint8_t *p_buff, size_t size)
{
    utl_cbf_t *buff = (active_buffer == UART_NUM_0) ? &uart_rx_buff_1 : &uart_rx_buff_0;
    
    size_t bytes_written = 0;
    
    for(size_t bytes_written = 0; bytes_written < size; bytes_written++)
    {
        if(utl_cbf_get(buff, &p_buff[bytes_written]) == UTL_CBF_EMPTY)
        {
            ESP_LOGW(TAG, "Aborting NOW. %d bytes have been written!", bytes_written);
            (active_buffer == UART_NUM_0) ? (cbf_0_ready_to_fill = true) : (cbf_1_ready_to_fill = true);
            break;
        }
    }

    return bytes_written;
}

static void uart_periph_process_uart_rx(uint8_t c)
{
    utl_cbf_t *buff = (active_buffer == UART_BUFFER_0) ? &uart_rx_buff_0 : &uart_rx_buff_1;

    if(utl_cbf_put(buff, c) != UTL_CBF_OK)
    {
        if(active_buffer == UART_BUFFER_0)
        {
            cbf_0_ready_to_fill = false;
            cbf_1_ready_to_fill = true;
            active_buffer = UART_BUFFER_1;
            utl_cbf_put(&uart_rx_buff_1, c);
            ESP_LOGI(TAG, "%s cheio, trocando para %s!",STRINGFY(UART_BUFFER_0), STRINGFY(UART_BUFFER_1));
        }
        else
        {
            cbf_0_ready_to_fill = true;
            cbf_1_ready_to_fill = false;
            active_buffer = UART_BUFFER_0;
            utl_cbf_put(&uart_rx_buff_0, c);
            ESP_LOGI(TAG, "%s cheio, trocando para %s!",STRINGFY(UART_BUFFER_1), STRINGFY(UART_BUFFER_0));
        }
    }
}

static bool uart_periph_record_data_SD(uint8_t *p_buff, size_t size, const char *file_name)
{

}