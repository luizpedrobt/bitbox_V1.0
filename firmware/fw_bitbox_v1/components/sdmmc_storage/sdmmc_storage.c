#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>

#include "sdmmc_storage.h"
#include "driver/sdmmc_default_configs.h"
#include "sdkconfig.h"
#include "esp_err.h"
#include "esp_log.h"
#include "soc/gpio_num.h"
#include "esp_log.h"
#include "ff.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "sdmmc_cmd.h"

sdmmc_host_t host;
sdmmc_slot_config_t slot_cfg;
sdmmc_card_t *card;

static const char *TAG = "SDMMC_STORAGE";
static char path[MAX_FILES_SUPPORTEDS] = { 0 };

static void sdmmc_cfg_host(void)
{
    host = (sdmmc_host_t)SDMMC_HOST_DEFAULT();
    host.max_freq_khz = 40000;

    slot_cfg = (sdmmc_slot_config_t)SDMMC_SLOT_CONFIG_DEFAULT();
    slot_cfg.clk = SDMMC_CLK_PIN;
    slot_cfg.cmd = SDMMC_CMD_PIN;
    slot_cfg.d0  =  SDMMC_D0_PIN;
    slot_cfg.d1  =  SDMMC_D1_PIN;
    slot_cfg.d2  =  SDMMC_D2_PIN;
    slot_cfg.d3  =  SDMMC_D3_PIN;
    slot_cfg.width = 4;
} 

static bool sdmmc_mount_device(void)
{
    esp_err_t ret;

    sdmmc_cfg_host();

    esp_vfs_fat_sdmmc_mount_config_t mount_config =
    {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024,
    };

    ESP_LOGI(TAG, "Monting SD card...");
    ret = esp_vfs_fat_sdmmc_mount(MOUNT_POINT, &host,
                                &slot_cfg,  &mount_config,
                                   &card);
    
    if(ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed mount SD card (%s)", esp_err_to_name(ret));
        return false;
    }

    ESP_LOGI(TAG, "SD card mounted. Type: %s", card->cid.name);

    sdmmc_card_print_info(stdout, card);

    FILE* test_file = fopen(MOUNT_POINT"/test.txt", "w");
    if(test_file == NULL) 
    {
        ESP_LOGE(TAG, "Falha no teste de escrita no SD card");
        return false;
    }
    fprintf(test_file, "Teste de escrita SD card\n");
    fclose(test_file);

    remove(MOUNT_POINT"/test.txt");

    ESP_LOGI(TAG, "SD card verificado e pronto para uso");
    return true;
}

static bool sdmmc_stor_get_log_name(char *output_name, size_t len)
{
    FILE *f;
    uint8_t idx = 1;

    while (1)
    {
        snprintf(output_name, len, PATH_BIN, idx);

        f = fopen(output_name, "rb");
        if (f == NULL)
        {
            ESP_LOGI(TAG, "Arquvo log%d.bin não existe! Criando...", idx);
            return true;
        }

        // existe → fecha e tenta o próximo 
        fclose(f);
        idx++;

        if(idx == MAX_FILES_SUPPORTEDS)
        {
            ESP_LOGE(TAG, "Número máximo de arquivos alcançado! Extraia os arquivos do cartão!");
            return false;
        }
    }
}

bool sdmmc_stor_init(void)
{
    sdmmc_cfg_host();
    if(!sdmmc_mount_device())
    {
        ESP_LOGE(TAG, "Erro ao montar device SDMMC!");
        return false;
    }

    if(!sdmmc_stor_get_log_name(path, sizeof(path)))
    {
        ESP_LOGE(TAG, "Erro ao abrir arquivo!");
        return false;
    }
      
    ESP_LOGI(TAG, "SDMMC montado com sucesso!");
    return true;
}

bool sdmmc_stor_record_data_bin(const void *p_data, size_t size)
{
    FILE *f = fopen(path, "ab");
    if(f == NULL)
    {
        ESP_LOGE(TAG, "Erro ao abrir arquivo!");
        return false;
    }

    size_t bytes_written = fwrite(p_data, 1, size, f);
    ESP_LOGI(TAG, "%zu bytes gravados com sucesso em %s", bytes_written, path);

    fclose(f);

    return true;
}
