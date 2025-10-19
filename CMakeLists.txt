cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

project(project_gofkucam
    DESCRIPTION "Project GofkuCam"
    LANGUAGES CXX
)

# Update C++ standard
set(CMAKE_CXX_STANDARD 20)

# Imported CMake modules
set (CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

set(QPCPP_CFG_KERNEL QXK)
set(QPCPP_CFG_PORT posix)
set(QPCPP_CFG_GUI OFF)
set(QPCPP_CFG_DEBUG OFF)
set(QPCPP_CFG_VERBOSE OFF)

add_subdirectory(extern/json)
add_subdirectory(extern/qpcpp)
add_subdirectory(extern/spdlog)
#add_subdirectory(extern/libtorch)
add_subdirectory(src)

