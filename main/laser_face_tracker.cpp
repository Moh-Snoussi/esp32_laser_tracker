#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp_system.h>
#include "dl_image.hpp"
#include <iostream>
#include "human_face_detect_msr01.hpp"
#include "img_converters.h"
#include "dl_tool.hpp"
#include "dl_image.hpp"
#include "wrappers/camera/camera.h"
#include "wrappers/servo/servo.h"
#include "wrappers/wifi/wifi.h"

#ifdef CONFIG_USE_CAM_SERVER
#include "wrappers/server/server.h"
#endif

#ifdef CONFIG_USE_SDCARD
#include "wrappers/sd/sd.h"
#endif

/**
 * @brief Pin connected to the lazer, can be configured in menuconfig: idf.py menuconfig
 */
static const gpio_num_t LAZER_PIN = (gpio_num_t)CONFIG_LAZER_RELAY_PIN;

static const gpio_num_t SERVO_SIGNAL_PIN = (gpio_num_t)CONFIG_SERVO_SIGNAL_PIN;

/**
 * @brief Task handle for core a
 */
TaskHandle_t task_handle;

static QueueHandle_t xQueueFrameO = NULL;

servo_config_t servo_cfg = {
    .max_angle = 180,
    .min_width_us = 1000,
    .max_width_us = 2000,
    .freq = 50,
    .channels = {
        .servo_pin = {SERVO_SIGNAL_PIN},
        // channel should not conflict with other channels, like that of the camera
        .ch = {LEDC_CHANNEL_0},
    }};

Servo servo(servo_cfg, false);

/**
 * @brief Capture the image and send it to the queue, with a delay of 30ms
 */
void core_a(void *pvParameters)
{

  xQueueFrameO = xQueueCreate(2, sizeof(camera_fb_t *));

  servo.turnTo(90.0F, LEDC_CHANNEL_0);

  while (true)
  {
    if (uxQueueSpacesAvailable(xQueueFrameO) > 0)
    {
      camera_fb_t *frame = Camera::capture();
      xQueueSend(xQueueFrameO, &frame, portMAX_DELAY);
    }
    vTaskDelay(30 / portTICK_PERIOD_MS);
  }
}

void core_b(void *pvParameters)
{
  /**
   * @brief Fade tracker, if no face is detected for 3 frames, turn off the lazer
  */
  int fade_tracker = 0;
  int fade_threshold = 4;
  float senstivity = 0.7F;
  HumanFaceDetectMSR01 detector(0.2F, 0.5F, 1, 0.4F);
  camera_fb_t *frame = nullptr;
  while (true)
  {
    xQueueReceive(xQueueFrameO, &frame, portMAX_DELAY);
    if (frame != nullptr)
    {
      std::list<dl::detect::result_t> &results = detector.infer<uint16_t>((uint16_t *)frame->buf, {(int)frame->height, (int)frame->width, 3});

      fade_tracker++;
      if (results.size() == 0 && fade_tracker >= fade_threshold)
      {
        gpio_set_level(LAZER_PIN, 0);
        continue;
      }

      int i = 0;
      for (std::list<dl::detect::result_t>::iterator prediction = results.begin(); prediction != results.end(); prediction++, i++)
      {

#ifdef CONFIG_USE_SDCARD
        SD::save_image(frame, "test");
#endif

        ESP_LOGI("face at", "x: %i, y: %i, w: %i, h: %i", prediction->box[0], prediction->box[1], prediction->box[2], prediction->box[3]);
        // invert angle
        // ESP_LOGI("servo angle set to", "angle: %f, target_angle: %f", angle, target_angle);
        float mid_x = (prediction->box[0] + prediction->box[2]) / 2;
        float angle = (mid_x / frame->width) * 180;
        float target_angle = 90 + ((90 - angle) * senstivity);
        esp_err_t success = servo.turnTo(target_angle, LEDC_CHANNEL_0);
        gpio_set_level(LAZER_PIN, 1); // turn on lazer
        fade_tracker = 0;

#ifdef CONFIG_USE_CAM_SERVER
        Server::imageBuff = frame->buf;
#endif
      }
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

extern "C" void app_main()
{
  ESP_LOGI("main", "LASER_PIN: %i", LAZER_PIN);

  gpio_config_t io_conf = {
      .pin_bit_mask = (1ULL << LAZER_PIN),
      .mode = GPIO_MODE_OUTPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };

  gpio_config(&io_conf);

#ifdef CONFIG_USE_CAM_SERVER
  // wifi credentials are adjusted in menuconfig
  if (Wifi::Connect())
  {
    Server::startServer();
  }
#endif

  static const char *TAGX = "main";
  ESP_LOGI(TAGX, "Starting core a");
  xTaskCreatePinnedToCore(core_a, TAGX, 3 * 1024, NULL, 5, NULL, 0);
  ESP_LOGI(TAGX, "Starting core b");
  xTaskCreatePinnedToCore(core_b, TAGX, 3 * 1024, NULL, 5, NULL, 1);
}
