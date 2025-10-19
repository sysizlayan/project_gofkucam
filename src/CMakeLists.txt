# CMake config for compiled library code and executable ROS node.

#CURRENT_DIR=$(cd "$(dirname "$0")" && pwd)

set(THREADS_PREFER_PTHREAD_FLAG ON)
set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_C_COMPILER "clang")

# Remove the forced Debug mode
#set(CMAKE_BUILD_TYPE Debug)

# Add configuration types
set(CMAKE_CONFIGURATION_TYPES Debug Release)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Set compile flags for different build types
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")

if(NOT ONNXRUNTIME_DIR)
    set(ONNXRUNTIME_DIR "${CMAKE_SOURCE_DIR}/extern/onnxruntime-linux-x64-1.20.1")
endif()

message(STATUS "ONNX runtime directory: ${ONNXRUNTIME_DIR}")

# Update C++ standard
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_PREFIX_PATH ../extern/libtorch)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

find_package(OpenCV REQUIRED)
find_package(spdlog REQUIRED)
find_package(CURL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED glib-2.0)
#find_package(Torch REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS})
message(STATUS "OpenCV library status:")
message(STATUS "    config: ${OpenCV_DIR}")
message(STATUS "    version: ${OpenCV_VERSION}")
message(STATUS "    libraries: ${OpenCV_LIBS}")
message(STATUS "    include path: ${OpenCV_INCLUDE_DIRS}")

include_directories(${OpenCV_INCLUDE_DIRS})

find_package(Threads REQUIRED)

# Quantum Leaps, QM tool generated files
file(GLOB QM_GENERATED_SOURCES
     "qm_generated/*.cpp"
)
message(STATUS "QM Generated Files: ${QM_GENERATED_SOURCES}")
file(GLOB GOFKUCAM_SOURCES
     "*.cpp"
)
message(STATUS "Source Files: ${GOFKUCAM_SOURCES}")

add_library(gofkucam_lib
  STATIC
  ${GOFKUCAM_SOURCES}
  ${QM_GENERATED_SOURCES}
)

target_include_directories(gofkucam_lib
    PUBLIC
    ../include
    ../include/qm_generated
    ../extern/qp-bundle/qpcpp/include
    ../extern/qp-bundle/qpcpp/src
    ../extern/qp-bundle/qpcpp/ports/posix
    ../extern/json/include
    ${ONNXRUNTIME_DIR}/include
    ${CURL_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
)

target_compile_options(gofkucam_lib
    PRIVATE
    -Wno-sign-compare
    -Wno-infinite-recursion
) 
    
target_link_libraries(gofkucam_lib
    PUBLIC
    qpcpp
    spdlog::spdlog
    ${OpenCV_LIBS}
    CURL::libcurl
    ${GLIB_LIBRARIES}
    #"${TORCH_LIBRARIES}"
    ${ONNXRUNTIME_DIR}/lib/libonnxruntime.so
)

# Target: The executable for the gofkucam
add_executable(gofkucam
    ../main.cpp
)

target_compile_features(gofkucam
    PRIVATE
    cxx_std_20
)

# Add build-specific compile definitions
target_compile_definitions(gofkucam
    PRIVATE
    $<$<CONFIG:Debug>:DEBUG_MODE>
    $<$<CONFIG:Release>:RELEASE_MODE>
)

# Add build-specific compile options
target_compile_options(gofkucam
    PRIVATE
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O3 -march=native>
)

target_include_directories(gofkucam
    PUBLIC
    include
)
target_link_libraries(gofkucam
    PUBLIC
    gofkucam_lib
    Threads::Threads
    spdlog::spdlog
    CURL::libcurl
    ${GLIB_LIBRARIES}
)