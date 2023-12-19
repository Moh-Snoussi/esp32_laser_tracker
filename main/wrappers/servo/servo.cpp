#include "servo.h"
#include "./../../../custom_components/iot_servo/include/iot_servo.h"

#include "esp_log.h"

servo_config_t default_cfg = {
    .max_angle = 180,
    .min_width_us = 10,
    .max_width_us = 5000,
    .freq = 50,
    .timer_number = LEDC_TIMER_0,
    .channels = {
        .servo_pin = {GPIO_NUM_4},
        // channel_0 is used by the camera
        .ch = {LEDC_CHANNEL_2},
    },
    .channel_number = 1};

/**
 * @brief  Initialize the servo
 * if deinit is true, then the servo will be initialized and deinitialized after every turnTo call
 */
Servo::Servo(servo_config_t configX, bool deinit = false) : log_tag("Servo"), deinit(deinit)
{
    this->config = hydrateConfig(configX);
    if (!deinit)
    {
        esp_err_t success = iot_servo_init(LEDC_LOW_SPEED_MODE, &config);
        ESP_LOGI(log_tag, "Servo initialized with status %s", success == ESP_OK ? "OK" : "ERROR");
    }
}

void Servo::free()
{
    esp_err_t success = iot_servo_deinit(LEDC_LOW_SPEED_MODE);
    if (success != ESP_OK)
    {
        ESP_LOGE(log_tag, "Failed to deinitialize servo");
    }
}

servo_config_t Servo::hydrateConfig(servo_config_t servo_cfg)
{

    if (servo_cfg.max_angle != NULL)
    {
        default_cfg.max_angle = servo_cfg.max_angle;
    }

    if (servo_cfg.min_width_us != NULL)
    {
        default_cfg.min_width_us = servo_cfg.min_width_us;
    }

    if (servo_cfg.max_width_us != NULL)
    {
        default_cfg.max_width_us = servo_cfg.max_width_us;
    }

    if (servo_cfg.freq != NULL)
    {
        default_cfg.freq = servo_cfg.freq;
    }

    if (servo_cfg.channel_number != NULL)
    {
        default_cfg.channel_number = servo_cfg.channel_number;
    }

    if (servo_cfg.timer_number != NULL)
    {
        default_cfg.timer_number = servo_cfg.timer_number;
    }

    default_cfg.channels = servo_cfg.channels;

    // get channel_number from servo_pin and ch
    uint8_t n_ch = 0;

    for (uint8_t i = 0; i < LEDC_CHANNEL_MAX; i++)
    {
        if (servo_cfg.channels.servo_pin[i] != NULL)
        {
            n_ch++;
        }
    }

    // channel_number  from the length of servo_pin and ch
    default_cfg.channel_number = n_ch;

    ESP_LOGI(log_tag, "Setting servo config, max_angle: %d, min_width_us: %d, max_width_us: %d, freq: %d, channel_number: %d, timer_number: %d",
             default_cfg.max_angle, default_cfg.min_width_us, default_cfg.max_width_us, default_cfg.freq, default_cfg.channel_number, default_cfg.timer_number);

    return default_cfg;
}

esp_err_t Servo::turnTo(float angle, ledc_channel_t channel)
{
    // angle is float
    if (deinit)
    {
        esp_err_t success = iot_servo_init(LEDC_LOW_SPEED_MODE, &this->config);
        if (success != ESP_OK)
        {
            ESP_LOGE(log_tag, "Failed to initialize servo");
        }
    }
    ESP_LOGD(log_tag, "Setting servo angle to %f", angle);
    esp_err_t success = iot_servo_write_angle(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, angle);
    if (success != ESP_OK)
    {
        ESP_LOGE(log_tag, "Failed to set servo angle");
    }

    if (deinit)
    {
        esp_err_t success = iot_servo_deinit(LEDC_LOW_SPEED_MODE);
        gpio_reset_pin(this->config.channels.servo_pin[0]);
        if (success != ESP_OK)
        {
            ESP_LOGE(log_tag, "Failed to deinitialize servo");
        }
    }

    ESP_LOGI(log_tag, "Servo angle set to %f", angle);

    return success;
}
