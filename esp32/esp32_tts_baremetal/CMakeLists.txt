cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# 业务组件：共享驱动与纯逻辑层（两版项目共用，唯一维护点）
# 注意：EXTRA_COMPONENT_DIRS 必须在 project() 之前设置
set(EXTRA_COMPONENT_DIRS ${CMAKE_CURRENT_LIST_DIR}/../components)
project(esp32_tts_baremetal)
