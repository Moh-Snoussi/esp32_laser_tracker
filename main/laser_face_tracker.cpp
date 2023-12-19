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
#include "wrappers/sd/sd.h"
#include "wrappers/server/server.h"
#include "wrappers/servo/servo.h"
#include "wrappers/wifi/wifi.h"

/**
 * @brief Pin connected to the lazer, can be configured in menuconfig: idf.py menuconfig
 */
static const gpio_num_t LAZER_PIN = (gpio_num_t)CONFIG_LAZER_PIN;

#ifndef CONFIG_USE_CAM_SERVER
#define CONFIG_USE_CAM_SERVER false
#endif

#ifndef CONFIG_USE_SDCARD
#define CONFIG_USE_SDCARD false
#endif



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
        .servo_pin = {GPIO_NUM_14},
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
  HumanFaceDetectMSR01 detector(0.2F, 0.5F, 1, 0.4F);
  camera_fb_t *frame = nullptr;
  while (true)
  {
    xQueueReceive(xQueueFrameO, &frame, portMAX_DELAY);
    if (frame != nullptr)
    {
      std::list<dl::detect::result_t> &results = detector.infer<uint16_t>((uint16_t *)frame->buf, {(int)frame->height, (int)frame->width, 3});

      if (results.size() == 0)
      {
        gpio_set_level(LAZER_PIN, 0);
        continue;
      }

      int i = 0;
      for (std::list<dl::detect::result_t>::iterator prediction = results.begin(); prediction != results.end(); prediction++, i++)
      {
        uint16_t mid_x = (prediction->box[0] + prediction->box[2]) / 2;
        float diff_x = mid_x - (frame->width / 2);
        float angle = diff_x / (float)frame->width * 180.0F;

        // invert angle
        float target_angle = 90 - angle;
        esp_err_t success = servo.turnTo(target_angle, LEDC_CHANNEL_0);
        gpio_set_level(LAZER_PIN, 1); // turn on lazer

        if (CONFIG_USE_CAM_SERVER)
        {
          Server::imageBuff = frame->buf;
        }
      }
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

extern "C" void app_main()
{
  // pin 15 is connected to a relay, which controls the lazer
  gpio_config_t io_conf = {
      .pin_bit_mask = (1ULL << 15),
      .mode = GPIO_MODE_OUTPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };

  gpio_config(&io_conf);

  if (CONFIG_USE_CAM_SERVER)
  {
    // wifi credentials are adjusted in menuconfig
    if (Wifi::Connect())
    {
      Server::startServer();
    }
  }

  static const char *TAGX = "main";
  ESP_LOGI(TAGX, "Starting core a");
  xTaskCreatePinnedToCore(core_a, TAGX, 3 * 1024, NULL, 5, NULL, 0);
  ESP_LOGI(TAGX, "Starting core b");
  xTaskCreatePinnedToCore(core_b, TAGX, 3 * 1024, NULL, 5, NULL, 1);
}
