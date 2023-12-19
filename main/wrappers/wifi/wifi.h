#ifndef WIFI_H
#define WIFI_H

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_event.h"

class Wifi
{
public:
    Wifi();

    const char *TAG;
    static bool Connect();

private:
    static Wifi &getInstance();
    static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
    EventGroupHandle_t s_wifi_event_group;
    int s_retry_num;
    void Init();
    void InitEventGroup();
    void InitWifiConfig();
    void StartWifi();
    bool AttemptConnection();
};

#endif // WIFI_H
