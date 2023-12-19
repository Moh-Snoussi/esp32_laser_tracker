#include "driver/gpio.h"
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

static esp_err_t init_sd_card(void)
{
  sdmmc_host_t host = SDMMC_HOST_DEFAULT();
  sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = false,
      .max_files = 3,
  };
  sdmmc_card_t *card;
  esp_err_t err = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);
  if (err != ESP_OK)
  {
    ESP_LOGI(TAG, "Failed to mount SD card VFAT filesystem. Error: %s", esp_err_to_name(err));
    return err;
  }

  ESP_LOGI(TAG, "SD card mounted");
  return ESP_OK;
}

static void save_image(camera_fb_t *frame, char *filename)
{

  ESP_LOGI(TAG, "Saving image with length: %zu", frame->len);
  // create a new file
  char *target = "/sdcard";
  sprintf(target, "%s/%s", target, filename);
  FILE *file = fopen("/sdcard/", "a");

  if (!file)
  {
    ESP_LOGE(TAG, "Failed to open file in writing mode");
    return;
  }

  // write test to file
  fwrite(frame->buf, 1, frame->len, file);

  fclose(file);

  ESP_LOGI(TAG, "Saved file to path: %s", filename);
}
