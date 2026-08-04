idf_component_register(
    SRCS "app_main.c" "rfid_task.c" "motor_task.c"
    INCLUDE_DIRS "."
    REQUIRES common
)
