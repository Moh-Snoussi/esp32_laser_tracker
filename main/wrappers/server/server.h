#ifndef MY_SERVER_H
#define MY_SERVER_H

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_http_server.h"
#include "esp_camera.h"

class Server {
public:
  static const char *TAG;

  static esp_err_t handleRoot(httpd_req_t *req);
  static esp_err_t getCurrentImage(httpd_req_t *req);
  static void startServer();
  static uint8_t *imageBuff;
  static bool isBusy;

private:
  static httpd_handle_t server;
  // Assuming camera_buf_t is declared elsewhere
};

#endif // MY_SERVER_H
