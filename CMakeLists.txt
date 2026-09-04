cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

project(project_gofkucam
    DESCRIPTION "Project GofkuCam"
    LANGUAGES CXX
)

# Update C++ standard
set(CMAKE_CXX_STANDARD 20)

# Imported CMake modules
set (CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(QPCPP_CFG_KERNEL QXK)
set(QPCPP_CFG_PORT posix)
set(QPCPP_CFG_GUI OFF)
set(QPCPP_CFG_DEBUG OFF)
set(PAHO_WITH_SSL FALSE)
set(PAHO_WITH_MQTT_C TRUE CACHE BOOL "" FORCE)

add_subdirectory(extern/json)
add_subdirectory(extern/paho.mqtt.cpp)
add_subdirectory(extern/qpcpp)
add_subdirectory(extern/spdlog)
add_subdirectory(src)
