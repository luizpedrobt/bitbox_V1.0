#pragma once

#include "uart_periph.h"
#include "gpio_peripheral.h"

#define STORAGE_NAMESPACE "storage"

#define XMACRO_SYS_CONFIG                                                        \
        X(0, UART_CFG, sys_config_uart_t, uart_cfg, "uart_cfg", app_config_uart) \
        X(1, GPIO_CFG, sys_config_gpio_t, gpio_cfg, "gpio_cfg", app_config_gpio) \
        X(2, NETW_CFG, sys_config_netw_t, netw_cfg, "netw_cfg", app_config_netw)

typedef enum sys_config_type_e
{
    #define X(IDX, PERIPH_CFG, ST_PERIPH, CFG_VAR, NAMESPACE, FUNC_PROT) PERIPH_CFG = IDX,
        XMACRO_SYS_CONFIG
    #undef X

    INV_CFG,
} sys_config_type_t;

typedef struct sys_config_uart_s
{
    uint8_t uart_cnt;
    uart_cfg_t uarts[UART_NUM_MAX];
} sys_config_uart_t; 

typedef struct sys_config_gpio_s
{
    uint8_t gpio_cnt;
    gpio_cfg_t gpios[GPIO_BOARD_MAX];
} sys_config_gpio_t;

typedef struct sys_config_netw_s
{
    char ssid[32];
    char pass[64];
} sys_config_netw_t;

#define X(IDX, PERIPH_CFG, ST_PERIPH, CFG_VAR, NAMESPACE, FUNC_PROT) esp_err_t FUNC_PROT##_save(const ST_PERIPH *CFG_VAR);
    XMACRO_SYS_CONFIG
#undef X

#define X(IDX, PERIPH_CFG, ST_PERIPH, CFG_VAR, NAMESPACE, FUNC_PROT) esp_err_t FUNC_PROT##_load(ST_PERIPH *CFG_VAR);
    XMACRO_SYS_CONFIG
#undef X

esp_err_t app_config_erase_all(void);

void app_config_main(void);

