#include "wifi_ap.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_check.h"
#include <string.h>

void wifi_ap_start(void)
{
    esp_err_t err;

    err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) return;

    /* 默认事件循环可能已被创建（重复进入配置模式时容错） */
    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) return;

    (void)esp_netif_create_default_wifi_ap();

    wifi_init_config_t wcfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&wcfg);
    if (err != ESP_OK) return;

    err = esp_wifi_set_mode(WIFI_MODE_AP);
    if (err != ESP_OK) return;

    wifi_config_t wc = {
        .ap = {
            .ssid     = "EV-Car-Setup",
            .ssid_len = 12,
            .channel  = 1,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .password = "12345678",
            .max_connection = 4,
        },
    };
    esp_wifi_set_config(WIFI_IF_AP, &wc);
    esp_wifi_start();
}
