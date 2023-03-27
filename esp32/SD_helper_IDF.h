#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "preferencesCLI.h"

static const char *SD_TASK_TAG = "SD_TASK";

// Pin assignments can be set in menuconfig, see "SD SPI Example Configuration" menu.
// You can also change the pin assignments here by changing the following 4 lines.
#define PIN_NUM_MISO 4
#define PIN_NUM_MOSI 18
#define PIN_NUM_CLK 5
#define PIN_NUM_CS (gpio_num_t)19

extern sdmmc_card_t *SD;

bool sd_init(sdmmc_card_t *card)
{
    esp_err_t ret;
    ESP_LOGI(SD_TASK_TAG, "Initializing SD card");

    const char mount_point[] = MOUNT_POINT;

    ESP_LOGI(SD_TASK_TAG, "Using SPI peripheral");
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    ret = spi_bus_initialize((spi_host_device_t)host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK)
    {
        ESP_LOGE(SD_TASK_TAG, "Failed to initialize bus.");
        return false;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = (spi_host_device_t)host.slot;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024};

    ESP_LOGI(SD_TASK_TAG, "Mounting filesystem");
    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(SD_TASK_TAG, "Failed to mount filesystem. "
                                  "If you want the card to be formatted, set the CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        }
        else
        {
            ESP_LOGE(SD_TASK_TAG, "Failed to initialize the card (%s). "
                                  "Make sure SD card lines have pull-up resistors in place.",
                     esp_err_to_name(ret));
        }
        return false;
    }

    ESP_LOGI(SD_TASK_TAG, "Filesystem mounted");
    return true;
}

void append_file(sdmmc_card_t *card, const char *path, const char *message)
{
    FILE *f = fopen(path, "a");
    if (f == NULL)
    {
        ESP_LOGE(SD_TASK_TAG, "Failed to open file for writing");
        return;
    }

    fprintf(f, message, card->cid.name);
    fclose(f);
    ESP_LOGI(SD_TASK_TAG, "File written");
}

// void rename_file(const char *path, const char *new_path)
// {
//     // Check if destination file exists before renaming
//     struct stat st;
//     if (stat(new_path, &st) == 0)
//     {
//         // Delete it if it exists
//         unlink(new_path);
//     }

//     ESP_LOGI(SD_TASK_TAG, "Renaming file %s to %s", path, new_path);
//     if (rename(path, new_path) != 0)
//     {
//         ESP_LOGE(SD_TASK_TAG, "Rename failed");
//         return;
//     }
// }

// void read_file(const char *path)
// {
//     ESP_LOGI(SD_TASK_TAG, "Reading file %s", path);
//     FILE *f = fopen(path, "r");
//     if (f == NULL)
//     {
//         ESP_LOGE(SD_TASK_TAG, "Failed to open file for reading");
//         return;
//     }

//     // Read a line from file
//     char line[64];
//     fgets(line, sizeof(line), f);
//     fclose(f);

//     // Strip newline
//     char *pos = strchr(line, '\n');
//     if (pos) *pos = '\0';
//     ESP_LOGI(SD_TASK_TAG, "Read from file: '%s'", line);
// }