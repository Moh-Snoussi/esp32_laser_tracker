#include "esp_log.h"
#include "esp_camera.h"
#include "./camera.h"
#include "esp_random.h"
#include "img_converters.h"

typedef struct
{
    uint8_t *buf;
    size_t len;
} picture_t;

/**
 * @brief Construct a new Camera:: Camera object
 * @note The default values are taken from the esp_camera.h, the CONFIG_ values are defined in the ../Kconfig.projbuild
 */
Camera::Camera() : camera_config{
                       .pin_pwdn = CONFIG_PWDN,
                       .pin_reset = CONFIG_RESET,
                       .pin_xclk = CONFIG_XCLK,
                       .pin_sscb_sda = CONFIG_SDA,
                       .pin_sscb_scl = CONFIG_SCL,

                       .pin_d7 = CONFIG_D7,
                       .pin_d6 = CONFIG_D6,
                       .pin_d5 = CONFIG_D5,
                       .pin_d4 = CONFIG_D4,
                       .pin_d3 = CONFIG_D3,
                       .pin_d2 = CONFIG_D2,
                       .pin_d1 = CONFIG_D1,
                       .pin_d0 = CONFIG_D0,
                       .pin_vsync = CONFIG_VSYNC,
                       .pin_href = CONFIG_HREF,
                       .pin_pclk = CONFIG_PCLK,

                       // XCLK 20MHz or 10MHz
                       .xclk_freq_hz = CONFIG_XCLK_FREQ,
                       .ledc_timer = LEDC_TIMER_1,
                       .ledc_channel = LEDC_CHANNEL_4,

                       .pixel_format = PIXFORMAT_RGB565, // PIXFORMAT_RGB565, // YUV422,GRAYSCALE,RGB565,JPEG
                       .frame_size = FRAMESIZE_QVGA,     // QQVGA-UXGA Do not use sizes above QVGA when not JPEG

                       .jpeg_quality = 10, // 0-63 lower number means higher quality
                       .fb_count = 2,
                       .fb_location = CAMERA_FB_IN_PSRAM,
                       .grab_mode = CAMERA_GRAB_LATEST,
                   },
                   frame(nullptr), log_tag("WRAPPERS::CAMERA"), frame_perfix(esp_random()), counter(0){};

bool Camera::isInitialized = false;

// initialize the static instance of the camera
Camera Camera::instance{};

/**
 * @brief Get the Camera object, if it is not initialized, initialize it
 * Runs the esp_camera_init(&camera_config); only once, and returns the reference to the instance
 */
Camera &Camera::getInstance()
{
    if (isInitialized == false)
    {
        // unsure that the init function is called only once
        instance.init();
        isInitialized = true;
    }

    return instance;
};

/**
 * @brief Capture a frame from the camera, frees the previous frame if it exists, so there is no need to use esp_camera_fb_return
 *
 * @return camera_fb_t*
 */
camera_fb_t *Camera::capture()
{
    Camera &self = Camera::getInstance();

    if (self.frame != nullptr)
    {
        esp_camera_fb_return(self.frame);
        self.frame = nullptr;
    }

    self.frame = esp_camera_fb_get();
    if (!self.frame)
    {
        ESP_LOGE(self.log_tag, "Camera Capture Failed");
        return NULL;
    }

    ESP_LOGI(self.log_tag, "Picture extention: %s", self.frame_extension);
    ESP_LOGI(self.log_tag, "Frame size: %zu", self.frame->len);
    return self.frame;
};

char *Camera::generate_frame_name()
{
    char *frame_name = (char *)malloc(20);
    sprintf(frame_name, "%d_%d.%s", frame_perfix, counter, frame_extension);
    counter++;

    return frame_name;
};

/**
 * @brief Initialize the camera with the camera_config struct that is defined in the constructor
 *
 * @return esp_err_t
 *
 * @note This function is called only once, when the Camera::getInstance() is called for the first time
 */
esp_err_t Camera::init()
{
    switch (camera_config.pixel_format)
    {
    case PIXFORMAT_GRAYSCALE:
        frame_extension = "tiff";
        break;
    case PIXFORMAT_JPEG:
        frame_extension = "jpg";
        break;
    case PIXFORMAT_RGB565:
        frame_extension = "ppm";
        break;
    case PIXFORMAT_YUV422:
        frame_extension = "yuv";
        break;
    default:
        frame_extension = "raw";
        break;
    }

    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(log_tag, "Camera Init Failed");
        return err;
    }
    sensor_t *s = esp_camera_sensor_get();
    // initial sensors are flipped vertically and colors are a bit saturated
    s->set_brightness(s, 1);  // up the brightness just a bit
    return ESP_OK;
};
