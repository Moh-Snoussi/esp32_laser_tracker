#include "./../../../custom_components/iot_servo/include/iot_servo.h"

class Servo
{
public:
    Servo(servo_config_t servo_cfg, bool deinit);
    esp_err_t turnTo(float angle, ledc_channel_t channel);
    esp_err_t getAngle(uint8_t channel);
    void free();

private:
    char *log_tag;
    servo_config_t config;
    bool deinit;
    servo_config_t hydrateConfig(servo_config_t servo_cfg);
};
