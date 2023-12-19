#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"
#include "esp_camera.h"
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "esp_log.h"
#include "sd.h"


SD::SD() : log_tag("sd"), name(new char[20])
{
}


bool SD::isInitialized = false;
SD SD::instance{};

SD &SD::getInstance()
{
    if (isInitialized == false)
    {
        // unsure that the init function is called only once
        ESP_LOGI(instance.log_tag, "Initializing SD card");
        esp_err_t error = init();

        if (error != ESP_OK)
        {
            ESP_LOGE(instance.log_tag, "Failed to initialize SD card");
        } else {
            ESP_LOGI(instance.log_tag, "SD card initialized");
        }

        isInitialized = true;
    }

    return instance;
};


esp_err_t SD::init()
{
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = true,
        .max_files = 5,
        .allocation_unit_size = 8 * 1024
    };
    sdmmc_card_t *card;
    esp_err_t err = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);
    if (err != ESP_OK)
    {
        return err;
    }

    sdmmc_card_print_info(stdout, card);

    return ESP_OK;
}
int SD::counter = 0;
esp_err_t SD::save_image(camera_fb_t *frame, char *filename)
{
    // filename = "test3.jpg";
    SD &self = SD::getInstance();
    ESP_LOGI(self.log_tag, "Saving image with length: %zu", frame->len);
    // create a new file
    char *target;

    asprintf(&target, "/sdcard/x%i.jpg", SD::counter);
    SD::counter++;

    ESP_LOGI(self.log_tag, "Saving image to path: %s", target);

    // save the file
    FILE *file = fopen(target, "w");

    if (!file)
    {
        ESP_LOGE(self.log_tag, "Failed to open file in writing mode");
        return ESP_FAIL;
    }

    // write test to file
    fwrite(frame->buf, 1, frame->len, file);

    fclose(file);

    ESP_LOGI(self.log_tag, "Saved file to path: %s", filename);
    return ESP_OK;
}







