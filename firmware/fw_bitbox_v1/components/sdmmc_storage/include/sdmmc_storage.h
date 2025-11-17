#pragma once

#define SDMMC_CLK_PIN    GPIO_NUM_10
#define SDMMC_CMD_PIN    GPIO_NUM_11
#define SDMMC_D0_PIN     GPIO_NUM_9
#define SDMMC_D1_PIN     GPIO_NUM_8
#define SDMMC_D2_PIN     GPIO_NUM_13
#define SDMMC_D3_PIN     GPIO_NUM_12

#define MOUNT_POINT "/sdcard"
#define FILE_PATH_0 MOUNT_POINT"/buff0.txt"
#define FILE_PATH_1 MOUNT_POINT"/buff1.txt"

void sdmmc_stor_init(void);

bool sdmmc_stor_record_data_txt(const void *p_data, size_t size, const char *file_name);

bool sdmmc_stor_record_data_bin(const void *p_data, size_t size, const char *file_name);
