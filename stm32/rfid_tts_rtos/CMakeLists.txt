cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME rfid_tts_rtos)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
# User sources (业务代码，独立于 CubeMX 生成区，重新生成不会被覆盖)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    hardware/USART/BSP_USART.c
    hardware/DEBUG/Debug.c
    hardware/rfid_card/Card.c
    hardware/pwm/PWM.c
    hardware/LED/led.c
    hardware/ADC/BSP_ADC.c
    Task/rfid_logic.c
    Task/motor_logic.c
    Task/rfid_task.c
    Task/motor_control_task.c
    config/nvs_params.c
    config/param_cli.c
    config/isp_jump.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/hardware/USART
    ${CMAKE_CURRENT_SOURCE_DIR}/hardware/DEBUG
    ${CMAKE_CURRENT_SOURCE_DIR}/hardware/rfid_card
    ${CMAKE_CURRENT_SOURCE_DIR}/hardware/pwm
    ${CMAKE_CURRENT_SOURCE_DIR}/hardware/LED
    ${CMAKE_CURRENT_SOURCE_DIR}/hardware/ADC
    ${CMAKE_CURRENT_SOURCE_DIR}/Task
    ${CMAKE_CURRENT_SOURCE_DIR}/config
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
