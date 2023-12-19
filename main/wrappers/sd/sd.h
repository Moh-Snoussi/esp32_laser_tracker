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

#ifndef SD_H
#define SD_H

class SD
{
public:
    static SD &getInstance();
    static esp_err_t init();
    static esp_err_t save_image(camera_fb_t *frame, char *filename);

    SD();                      // Private constructor
    SD(const SD &);            // Private copy constructor
    SD &operator=(const SD &); // Private copy assignment operator

private:
    static bool isInitialized;
    static SD instance;
    const char *log_tag;
    static int counter;
    char *name;
};

#endif // SD_H
