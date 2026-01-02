#pragma once

#define SDMMC_CLK_PIN    GPIO_NUM_10
#define SDMMC_CMD_PIN    GPIO_NUM_11
#define SDMMC_D0_PIN     GPIO_NUM_9
#define SDMMC_D1_PIN     GPIO_NUM_8
#define SDMMC_D2_PIN     GPIO_NUM_13
#define SDMMC_D3_PIN     GPIO_NUM_12

#define MOUNT_POINT "/sdcard"
#define PATH_BIN MOUNT_POINT"/log%d.bin"
#define MAX_FILES_SUPPORTEDS 64


bool sdmmc_stor_init(void);

bool sdmmc_stor_record_data_bin(const void *p_data, size_t size);
