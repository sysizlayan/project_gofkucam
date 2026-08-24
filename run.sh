#!/bin/bash
# run.sh - Build and run the GofkuCam executable
set -e

# Run the executable from the build directory if it exists there, otherwise from project root
if [ -f build/src/gofkucam ]; then
    echo "Running build/gofkucam..."
    ./build/src/gofkucam
else
    echo "Executable gofkucam not found!"
    exit 1
fi
