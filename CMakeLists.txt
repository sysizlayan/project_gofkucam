cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

project(project_gofkucam
    DESCRIPTION "Project GofkuCam"
    LANGUAGES CXX
)

# Update C++ standard
set(CMAKE_CXX_STANDARD 23)

# Imported CMake modules
set (CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

# Build library and ROS executable
add_subdirectory(extern/json)
add_subdirectory(extern/qp-bundle)
#add_subdirectory(extern/libtorch)
add_subdirectory(src)
