# --- Build Stage ---
# Use Ubuntu 22.04 for the build environment
FROM ubuntu:22.04 AS builder

# Install dependencies required for building the project
# Using noninteractive mode to prevent prompts
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    build-essential \
    clang \
    clangd \
    cmake \
    wget \
    unzip \
    libspdlog-dev \
    libopencv-dev \
    libxcb-cursor0 \
    libcurl4-openssl-dev \
    libglib2.0-dev \
    && rm -rf /var/lib/apt/lists/*

# Set the working directory
WORKDIR /app

# Copy the entire project to the container
COPY . .

# Make scripts executable
RUN chmod +x build_docker.sh

# Build the project
RUN ./build_docker.sh

# --- Runtime Stage ---
# Use the same Ubuntu base for the final image
FROM ubuntu:22.04

# Install only essential runtime dependencies
RUN apt-get update && apt-get install -y \
    libgomp1 \
    && rm -rf /var/lib/apt/lists/*

# Set the working directory
WORKDIR /app

# Copy the compiled application from the builder stage
COPY --from=builder /app/build/src/gofkucam .

# Copy runtime libraries from the builder stage into a local lib folder
COPY --from=builder /app/extern/libtorch/lib ./lib/
COPY --from=builder /app/extern/onnxruntime-linux-x64-1.20.1/lib ./lib/
# Copy necessary OpenCV shared libraries from the builder
COPY --from=builder /usr/lib/x86_64-linux-gnu/libopencv_*.so.* ./lib/

# Copy configuration and resource files
COPY --from=builder /app/config ./config
COPY --from=builder /app/resources ./resources

# Add our local library path to the environment
ENV LD_LIBRARY_PATH=/app/lib

# Command to run the application
CMD ["./gofkucam"]
