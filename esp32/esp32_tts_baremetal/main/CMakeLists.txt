idf_component_register(
    SRCS "app_main.c" "rfid_process.c" "motor_process.c" "rfid_logic.c"
         "card_uart.c" "card_parse.c" "tts.c" "motor_drv.c" "led.c" "adc.c" "debug.c"
    INCLUDE_DIRS "."
)
