#include "esp_log.h"
#include "esp_camera.h"

#ifndef CAMERA_H
#define CAMERA_H

class Camera
{
public:
    static Camera &getInstance();
    static camera_fb_t *capture();
    esp_err_t init();

    Camera();                          // Private constructor
    Camera(const Camera &);            // Private copy constructor
    Camera &operator=(const Camera &); // Private copy assignment operator
    char *generate_frame_name();
private:
    camera_config_t camera_config;
    static bool isInitialized;
    static Camera instance;
    camera_fb_t *frame;
    // generate a name for the frame
    const char *log_tag;
    const int32_t frame_perfix;
    const char *frame_extension;
    int counter;
    char *name;
};

#endif // CAMERA_H
