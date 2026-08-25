idf_component_register(
    SRCS "card_uart.c" "card_parse.c" "tts.c" "motor_drv.c" "motor_logic.c"
         "rfid_logic.c" "led.c" "adc.c" "debug.c" "ws2812.c" "gbk_utf8.c"
         "nvs_params.c" "config_mode.c" "web_server.c" "wifi_ap.c"
    INCLUDE_DIRS "."
    EMBED_FILES "webpage.html"
    REQUIRES esp_driver_gpio esp_driver_uart esp_driver_ledc esp_driver_rmt
            esp_adc esp_timer freertos nvs_flash esp_wifi esp_http_server
            esp_netif esp_event cjson
)
