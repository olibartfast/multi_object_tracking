# Multi-stage Dockerfile for Multi-Object Tracking Project
# Stage 1: Build environment
FROM nvidia/cuda:12.2.0-devel-ubuntu22.04 AS builder

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    wget \
    ca-certificates \
    libopencv-dev \
    libeigen3-dev \
    libgoogle-glog-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

# Install ONNX Runtime with GPU support
WORKDIR /tmp
RUN wget https://github.com/microsoft/onnxruntime/releases/download/v1.19.2/onnxruntime-linux-x64-gpu-1.19.2.tgz \
    && tar -xzf onnxruntime-linux-x64-gpu-1.19.2.tgz \
    && mv onnxruntime-linux-x64-gpu-1.19.2 /opt/onnxruntime \
    && rm onnxruntime-linux-x64-gpu-1.19.2.tgz

# Set environment variables for ONNX Runtime
ENV ONNXRUNTIME_DIR=/opt/onnxruntime
ENV LD_LIBRARY_PATH=/opt/onnxruntime/lib:${LD_LIBRARY_PATH}

# Copy project files
WORKDIR /workspace
COPY . .

# Build the project
RUN rm -rf build && \
    cmake -G Ninja -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DDEFAULT_BACKEND=ONNX_RUNTIME \
    -DUSE_GSTREAMER=OFF \
    && cmake --build build --config Release

# Stage 2: Runtime environment
FROM nvidia/cuda:12.2.0-runtime-ubuntu22.04

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libopencv-core4.5d \
    libopencv-imgproc4.5d \
    libopencv-highgui4.5d \
    libopencv-video4.5d \
    libopencv-videoio4.5d \
    libgoogle-glog0v5 \
    libeigen3-dev \
    && rm -rf /var/lib/apt/lists/*

# Copy ONNX Runtime from builder
COPY --from=builder /opt/onnxruntime /opt/onnxruntime

# Set environment variables
ENV ONNXRUNTIME_DIR=/opt/onnxruntime
ENV LD_LIBRARY_PATH=/opt/onnxruntime/lib:${LD_LIBRARY_PATH}

# Create working directory
WORKDIR /app

# Copy the built executable and necessary files from builder
COPY --from=builder /workspace/build/multi_object_tracking /app/
COPY --from=builder /workspace/coco.names /app/
COPY --from=builder /workspace/trackers/BoTSORT/config /app/config/

# Create directories for models and videos
RUN mkdir -p /app/models /app/videos

# Set the entrypoint
ENTRYPOINT ["/app/multi_object_tracking"]

# Default command (can be overridden)
CMD ["--help"]
