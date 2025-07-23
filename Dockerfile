# STM32N6 Face Recognition System - Development Environment
# Multi-stage Docker build for embedded development and Python tools

# =============================================================================
# Stage 1: STM32 Development Environment (ARM GCC Cross-compilation)
# =============================================================================
FROM ubuntu:22.04 as stm32-dev

LABEL maintainer="STM32N6 Face Recognition System Contributors"
LABEL description="STM32N6 development environment with ARM GCC toolchain"

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC

# Install system dependencies
RUN apt-get update && apt-get install -y \
    # Build essentials
    build-essential \
    cmake \
    make \
    git \
    wget \
    curl \
    unzip \
    # ARM toolchain dependencies
    libc6-dev-i386 \
    lib32z1 \
    lib32ncurses6 \
    lib32stdc++6 \
    # Additional tools
    python3 \
    python3-pip \
    python3-venv \
    # STM32 tools dependencies
    libusb-1.0-0-dev \
    udev \
    # Cleanup
    && rm -rf /var/lib/apt/lists/*

# Install ARM GCC toolchain
ARG ARM_TOOLCHAIN_VERSION=12.3.rel1
ARG ARM_TOOLCHAIN_URL=https://developer.arm.com/-/media/Files/downloads/gnu/${ARM_TOOLCHAIN_VERSION}/binrel/arm-gnu-toolchain-${ARM_TOOLCHAIN_VERSION}-x86_64-arm-none-eabi.tar.xz

RUN cd /opt && \
    wget -q ${ARM_TOOLCHAIN_URL} -O arm-toolchain.tar.xz && \
    tar -xf arm-toolchain.tar.xz && \
    rm arm-toolchain.tar.xz && \
    mv arm-gnu-toolchain-${ARM_TOOLCHAIN_VERSION}-x86_64-arm-none-eabi arm-toolchain

# Add toolchain to PATH
ENV PATH="/opt/arm-toolchain/bin:${PATH}"

# Install STM32CubeProgrammer (CLI version)
# Note: This requires manual download from ST website due to license agreement
# Users should download and place STM32CubeProg-*.zip in the build context
COPY stm32cubeprog*.zip /tmp/ 2>/dev/null || echo "STM32CubeProgrammer not found - manual installation required"
RUN if [ -f /tmp/stm32cubeprog*.zip ]; then \
        cd /opt && \
        unzip -q /tmp/stm32cubeprog*.zip && \
        chmod +x STM32CubeProgrammer/bin/* && \
        rm /tmp/stm32cubeprog*.zip; \
    fi

# Set up development environment
WORKDIR /workspace

# Copy project files
COPY . /workspace/

# Create build directory
RUN mkdir -p /workspace/build

# Set environment variables for cross-compilation
ENV CC=arm-none-eabi-gcc
ENV CXX=arm-none-eabi-g++
ENV AR=arm-none-eabi-ar
ENV OBJCOPY=arm-none-eabi-objcopy
ENV OBJDUMP=arm-none-eabi-objdump
ENV SIZE=arm-none-eabi-size

# Verify toolchain installation
RUN arm-none-eabi-gcc --version && \
    arm-none-eabi-g++ --version

# Default command
CMD ["bash"]

# =============================================================================
# Stage 2: Python Tools Environment
# =============================================================================
FROM python:3.11-slim as python-tools

LABEL description="Python tools environment for STM32N6 Face Recognition System"

# Install system dependencies
RUN apt-get update && apt-get install -y \
    # OpenCV dependencies
    libgl1-mesa-glx \
    libglib2.0-0 \
    libsm6 \
    libxext6 \
    libxrender-dev \
    libgomp1 \
    # GUI dependencies (for Qt)
    libqt5gui5 \
    libqt5widgets5 \
    libqt5core5a \
    # USB and serial communication
    libusb-1.0-0 \
    # Network tools
    netcat-openbsd \
    # Cleanup
    && rm -rf /var/lib/apt/lists/*

# Set up Python environment
WORKDIR /app

# Copy Python requirements
COPY python_tools/requirements.txt /app/requirements.txt

# Install Python dependencies
RUN pip install --no-cache-dir -r requirements.txt

# Copy Python tools
COPY python_tools/ /app/

# Set up display for GUI applications (X11 forwarding)
ENV DISPLAY=:0

# Expose ports for communication
EXPOSE 8080 8081 3333

# Default command
CMD ["python", "run_ui.py"]

# =============================================================================
# Stage 3: Complete Development Environment
# =============================================================================
FROM stm32-dev as development

LABEL description="Complete STM32N6 development environment with Python tools"

# Install Python in the development environment
RUN apt-get update && apt-get install -y \
    python3-dev \
    python3-pip \
    python3-venv \
    # GUI support
    x11-apps \
    # Additional development tools
    gdb-multiarch \
    openocd \
    # Cleanup
    && rm -rf /var/lib/apt/lists/*

# Install Python tools
COPY python_tools/requirements.txt /workspace/python_tools/requirements.txt
RUN cd /workspace/python_tools && \
    pip3 install --no-cache-dir -r requirements.txt

# Create convenience scripts
RUN echo '#!/bin/bash\ncd /workspace && make clean && make -j$(nproc)' > /usr/local/bin/build-firmware && \
    chmod +x /usr/local/bin/build-firmware

RUN echo '#!/bin/bash\ncd /workspace/python_tools && python3 run_ui.py' > /usr/local/bin/run-python-ui && \
    chmod +x /usr/local/bin/run-python-ui

# Set up development user (optional, for better security)
ARG USER_ID=1000
ARG GROUP_ID=1000
RUN groupadd -g ${GROUP_ID} developer && \
    useradd -u ${USER_ID} -g ${GROUP_ID} -m -s /bin/bash developer && \
    usermod -aG dialout developer

# Set ownership
RUN chown -R developer:developer /workspace

# Switch to development user
USER developer

# Set up bash environment
RUN echo 'export PATH="/opt/arm-toolchain/bin:$PATH"' >> ~/.bashrc && \
    echo 'cd /workspace' >> ~/.bashrc

WORKDIR /workspace

# Default command
CMD ["bash"]

# =============================================================================
# Stage 4: CI/CD Environment
# =============================================================================
FROM stm32-dev as ci-cd

LABEL description="CI/CD environment for automated testing and building"

# Install additional CI/CD tools
RUN apt-get update && apt-get install -y \
    # Code analysis tools
    cppcheck \
    flawfinder \
    # Testing tools
    lcov \
    gcovr \
    # Documentation tools
    doxygen \
    graphviz \
    # Cleanup
    && rm -rf /var/lib/apt/lists/*

# Install Python testing dependencies
COPY python_tools/requirements.txt /workspace/python_tools/requirements.txt
RUN cd /workspace/python_tools && \
    pip3 install --no-cache-dir -r requirements.txt && \
    pip3 install --no-cache-dir pytest pytest-cov flake8 black

# Create CI/CD scripts
RUN echo '#!/bin/bash\n\
set -e\n\
echo "Building firmware..."\n\
cd /workspace\n\
make clean\n\
make -j$(nproc)\n\
echo "Running static analysis..."\n\
cppcheck --enable=all --xml --xml-version=2 Src/ Inc/ 2> cppcheck-report.xml || true\n\
flawfinder --csv Src/ Inc/ > flawfinder-report.csv || true\n\
echo "Running Python tests..."\n\
cd python_tools\n\
python3 -m pytest tests/ --cov=. --cov-report=xml || true\n\
echo "CI/CD pipeline completed"' > /usr/local/bin/ci-pipeline && \
    chmod +x /usr/local/bin/ci-pipeline

WORKDIR /workspace

# Default command for CI/CD
CMD ["ci-pipeline"]