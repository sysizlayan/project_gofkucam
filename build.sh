#!/bin/bash
# build.sh - Build script for GofkuCam C++ project using CMake
set -e

sudo apt install -y libspdlog-dev
BUILD_DIR=build

# Create build directory if it doesn't exist
mkdir -p $BUILD_DIR
cd $BUILD_DIR

# Configure project with CMake and export compile_commands.json
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Debug ..

# Build the project
make -j4

cd ..
