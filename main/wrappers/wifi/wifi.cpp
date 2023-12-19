#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "wifi.h"

#define ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_MAXIMUM_RETRY 5

#if CONFIG_ESP_WPA3_SAE_PWE_HUNT_AND_PECK
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HUNT_AND_PECK
#define EXAMPLE_H2E_IDENTIFIER ""
#elif CONFIG_ESP_WPA3_SAE_PWE_HASH_TO_ELEMENT
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HASH_TO_ELEMENT
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#elif CONFIG_ESP_WPA3_SAE_PWE_BOTH
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_BOTH
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#endif
#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

Wifi &Wifi::getInstance()
{
    static Wifi instance;
    return instance;
}

Wifi::Wifi() : TAG("wifi station"), s_retry_num(0)
{}

bool Wifi::Connect()
{
    Wifi &self = Wifi::getInstance();
    self.Init();
    self.InitEventGroup();
    self.InitWifiConfig();
    self.StartWifi();

    while (true)
    {
        if (self.AttemptConnection())
        {
            ESP_LOGI(self.TAG, "Connected to %s", ESP_WIFI_SSID);
            return true;
        }
        else
        {
            ESP_LOGI(self.TAG, "Failed to connect to %s. Retrying...", ESP_WIFI_SSID);
            vTaskDelay(pdMS_TO_TICKS(2000)); // Delay for 2 seconds before retrying
        }
    }
}

void Wifi::event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    Wifi &self = Wifi::getInstance();
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (self.s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            self.s_retry_num++;
            ESP_LOGI(self.TAG, "retry to connect to the AP");
        }
        else
        {
            xEventGroupSetBits(self.s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(self.TAG, "connect to the AP fail");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(self.TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        self.s_retry_num = 0;
        xEventGroupSetBits(self.s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void Wifi::Init()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
}

void Wifi::InitEventGroup()
{
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
}

void Wifi::InitWifiConfig()
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .threshold = {
                .authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
            },
            .sae_pwe_h2e = ESP_WIFI_SAE_MODE,
            // .sae_h2e_identifier = EXAMPLE_H2E_IDENTIFIER,

        },
    };

    strncpy((char *)wifi_config.sta.password, ESP_WIFI_PASS, 64); //copy at max 32 bytes over
    strncpy((char*)wifi_config.sta.ssid, ESP_WIFI_SSID, 32); //copy at max 32 bytes over


    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");
}

void Wifi::StartWifi()
{
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "wifi_init_sta finished.");
}

bool Wifi::AttemptConnection()
{
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           pdMS_TO_TICKS(portMAX_DELAY));

    if (bits & WIFI_CONNECTED_BIT)
    {
        return true;
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        s_retry_num++;
        ESP_LOGI(TAG, "Failed to connect. Retry %d of %d", s_retry_num, EXAMPLE_ESP_MAXIMUM_RETRY);
        if (s_retry_num >= EXAMPLE_ESP_MAXIMUM_RETRY)
        {
            ESP_LOGI(this->TAG, "Maximum retry count reached. Giving up.");
            return false;
        }
    }

    return false;
}
