#!/bin/bash
# build.sh - Build script for GofkuCam C++ project using CMake
set -e

BUILD_DIR=build

set -euo pipefail

CURRENT_DIR=$(cd "$(dirname "$0")" && pwd)

# Default values
ONNXRUNTIME_VERSION="${1:-1.20.1}"
ONNXRUNTIME_GPU="${2:-0}"

# Function to display usage
usage() {
    echo "Usage: $0 [ONNXRUNTIME_VERSION] [ONNXRUNTIME_GPU]"
    echo
    echo "This script downloads ONNX Runtime for the current platform and architecture and builds YOLOs-CPP."
    echo
    echo "Arguments:"
    echo "  ONNXRUNTIME_VERSION   Version of ONNX Runtime to download (default: 1.20.1)."
    echo "  ONNXRUNTIME_GPU       Whether to use GPU support (0 for CPU, 1 for GPU, default: 0)."
    echo
    echo "Examples:"
    echo "  $0 1.20.1 0          # Downloads ONNX Runtime v1.20.1 for CPU."
    echo "  $0 1.16.3 1          # Downloads ONNX Runtime v1.16.3 for GPU."
    echo
    exit 1
}

# Show usage if help is requested
if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    usage
fi

# Detect platform and architecture
platform=$(uname -s)
architecture=$(uname -m)

case "$platform" in
Darwin*)
    ONNXRUNTIME_PLATFORM="osx"
	ONNXRUNTIME_ARCHIVE_EXTENSION="tgz"
    ;;
Linux*) 
    ONNXRUNTIME_PLATFORM="linux"
	ONNXRUNTIME_ARCHIVE_EXTENSION="tgz"
    ;;
MINGW*) 
    ONNXRUNTIME_PLATFORM="win"
	ONNXRUNTIME_ARCHIVE_EXTENSION="zip"
    ;;
*)
    echo "Unsupported platform: $platform"
    exit 1
    ;;
esac

# Determine ONNX Runtime architecture
case "$architecture" in
aarch64|arm64)
    ONNXRUNTIME_ARCH="aarch64"
    ;;
x86_64)
    ONNXRUNTIME_ARCH="x64"
    ;;
arm*)
    ONNXRUNTIME_ARCH="arm"
    ;;
i*86)
    ONNXRUNTIME_ARCH="x86"
    ;;
*)
    echo "Unsupported architecture: $architecture"
    exit 1
    ;;
esac

# Set the correct ONNX Runtime download filename
ONNXRUNTIME_FILE="onnxruntime-${ONNXRUNTIME_PLATFORM}-${ONNXRUNTIME_ARCH}"
ONNXRUNTIME_DIR="${CURRENT_DIR}/extern/onnxruntime-${ONNXRUNTIME_PLATFORM}-${ONNXRUNTIME_ARCH}"

if [[ "$ONNXRUNTIME_GPU" -eq 1 ]]; then
    ONNXRUNTIME_FILE="${ONNXRUNTIME_FILE}-gpu"
    ONNXRUNTIME_DIR="${ONNXRUNTIME_DIR}-gpu"
fi

ONNXRUNTIME_FILE="${ONNXRUNTIME_FILE}-${ONNXRUNTIME_VERSION}.${ONNXRUNTIME_ARCHIVE_EXTENSION}"
ONNXRUNTIME_DIR="${ONNXRUNTIME_DIR}-${ONNXRUNTIME_VERSION}"
ONNXRUNTIME_URL="https://github.com/microsoft/onnxruntime/releases/download/v${ONNXRUNTIME_VERSION}/${ONNXRUNTIME_FILE}"

# Function to download and extract ONNX Runtime
download_onnxruntime() {
    echo "Downloading ONNX Runtime from $ONNXRUNTIME_URL ..."
    
    if ! curl -L -C - -o "${ONNXRUNTIME_FILE}" "$ONNXRUNTIME_URL"; then
        echo "Error: Failed to download ONNX Runtime."
        exit 1
    fi

    echo "Extracting ONNX Runtime ..."
	if [[ "${ONNXRUNTIME_ARCHIVE_EXTENSION}" = "tgz" ]]; then
		if ! tar -zxvf "${ONNXRUNTIME_FILE}" -C "$CURRENT_DIR"; then
			echo "Error: Failed to extract ONNX Runtime."
			exit 1
		fi
	elif [[ "${ONNXRUNTIME_ARCHIVE_EXTENSION}" = "zip" ]]; then
		if ! unzip "${ONNXRUNTIME_FILE}" -d "$CURRENT_DIR"; then
			echo "Error: Failed to extract ONNX Runtime."
			exit 1
		fi
	else
		echo "Error: Failed to extract ONNX Runtime."
		exit 1
	fi

    rm -f "${ONNXRUNTIME_FILE}"
}

if [ ! -d "$ONNXRUNTIME_DIR" ]; then
    download_onnxruntime
else
    echo "ONNX Runtime already exists. Skipping download."
fi
# Create build directory if it doesn't exist
mkdir -p $BUILD_DIR
cd $BUILD_DIR

# Configure project with CMake and export compile_commands.json
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Debug ..

# Build the project
make -j4

cd ..
