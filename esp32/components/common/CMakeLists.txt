idf_component_register(
    SRCS "card_uart.c" "card_parse.c" "tts.c" "motor_drv.c" "motor_logic.c"
         "rfid_logic.c" "led.c" "adc.c" "debug.c" "ws2812.c" "gbk_utf8.c"
         "nvs_params.c"
    INCLUDE_DIRS "."
    REQUIRES esp_driver_gpio esp_driver_uart esp_driver_ledc esp_driver_rmt
            esp_adc esp_timer freertos nvs_flash
)
