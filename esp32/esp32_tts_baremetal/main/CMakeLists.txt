idf_component_register(
    SRCS "app_main.c" "rfid_process.c" "motor_process.c"
    INCLUDE_DIRS "."
    REQUIRES common
)
