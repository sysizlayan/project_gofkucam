#!/bin/bash
# build.sh - Build script for GofkuCam C++ project using CMake
set -e

BUILD_DIR=build

# Create build directory if it doesn't exist
mkdir -p $BUILD_DIR
cd $BUILD_DIR

# Configure project with CMake and export compile_commands.json
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..

# Build the project
cmake --build .

cd ..
echo "Build successful. Binaries are in $BUILD_DIR."
