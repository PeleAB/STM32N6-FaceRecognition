# Docker Development Environment

This directory contains Docker configurations and convenience scripts for the STM32N6 Face Recognition System.

## Quick Start

### 1. Initial Setup
```bash
# Run the setup script
./docker-scripts/setup.sh
```

This will:
- Check Docker installation
- Build required Docker images
- Create convenience scripts
- Set up environment configuration

### 2. Start Development Environment
```bash
# Interactive development environment with full toolchain
./docker-scripts/dev.sh
```

This provides:
- ARM GCC cross-compilation toolchain
- STM32 development tools
- Python environment
- Source code mounted for editing

### 3. Build Firmware
```bash
# Build firmware in containerized environment
./docker-scripts/build.sh
```

### 4. Python Tools UI
```bash
# Start web-based Python tools interface
./docker-scripts/python-ui.sh
# Access at: http://localhost:8080
```

### 5. Jupyter Lab
```bash
# Start Jupyter Lab for notebook exercises
./docker-scripts/jupyter.sh
# Access at: http://localhost:8888
```

## Docker Services

### Available Services

| Service | Purpose | Access |
|---------|---------|--------|
| `development` | Complete dev environment | Interactive shell |
| `python-tools` | Python GUI and tools | http://localhost:8080 |
| `stm32-build` | Firmware build only | Command execution |
| `ci-cd` | Automated testing | Command execution |
| `jupyter` | Notebook server | http://localhost:8888 |
| `docs` | Documentation | http://localhost:8090 |

### Manual Docker Commands

```bash
# Start specific service
docker-compose up <service-name>

# Run interactive session
docker-compose run --rm <service-name> bash

# Build specific service
docker-compose build <service-name>

# View logs
docker-compose logs <service-name>

# Stop all services
docker-compose down
```

## Configuration

### Environment Variables (.env file)

```bash
# User permissions
USER_ID=1000
GROUP_ID=1000

# Display (for GUI apps)
DISPLAY=:0

# Port configurations
PYTHON_PORT=8080
JUPYTER_PORT=8888
DOCS_PORT=8090
```

### Volume Mounts

| Volume | Purpose | Persistence |
|--------|---------|-------------|
| Source code | Live editing | Host directory |
| `build-cache` | Build artifacts | Docker volume |
| `arm-toolchain-cache` | Toolchain cache | Docker volume |
| `pip-cache` | Python packages | Docker volume |

## Development Workflow

### 1. Code Development
```bash
# Start development environment
./docker-scripts/dev.sh

# Inside container:
cd /workspace
# Edit code with your host editor
# Build: make clean && make -j$(nproc)
# Debug: gdb-multiarch build/Project.elf
```

### 2. Python Tools Development
```bash
# Start Python environment
docker-compose run --rm python-tools bash

# Inside container:
cd /app
python run_ui.py
# or
pytest tests/
```

### 3. Testing and CI
```bash
# Run full CI pipeline
./docker-scripts/ci.sh

# Manual testing
docker-compose run --rm ci-cd bash
# Inside container: run specific tests
```

## Hardware Connection

### STM32 Programming

The development environment supports STM32 programming through USB:

1. **Connect STM32N6570-DK** via USB
2. **Verify connection**:
   ```bash
   # Inside development container
   lsusb | grep STMicroelectronics
   ```
3. **Flash firmware**:
   ```bash
   make flash
   # or
   STM32_Programmer_CLI -c port=SWD -w build/Project.bin 0x70100000
   ```

### USB Permissions

If USB devices are not accessible:

```bash
# Add user to dialout group
sudo usermod -aG dialout $USER

# Add udev rules (as shown in setup.sh)
sudo nano /etc/udev/rules.d/99-stlink-v2.rules

# Reload rules
sudo udevadm control --reload-rules
sudo udevadm trigger
```

## Troubleshooting

### Common Issues

#### Docker Build Fails
```bash
# Clean and rebuild
docker system prune -f
docker-compose build --no-cache development
```

#### USB Device Not Found
```bash
# Check USB permissions
ls -la /dev/bus/usb/
# Should be accessible by user

# Check STM32 connection
lsusb | grep STMicroelectronics
```

#### GUI Applications Don't Display
```bash
# Enable X11 forwarding
xhost +local:docker

# Check DISPLAY variable
echo $DISPLAY
# Should be :0 or similar
```

#### Build Errors
```bash
# Check ARM toolchain
arm-none-eabi-gcc --version

# Verify source code mount
ls -la /workspace
# Should show project files
```

### Container Management

```bash
# Remove all containers and volumes
./docker-scripts/clean.sh

# Check resource usage
docker system df

# View running containers
docker ps

# Enter running container
docker exec -it stm32n6-dev bash
```

## Performance Tips

### Build Optimization
- Use `make -j$(nproc)` for parallel builds
- Build cache is preserved in Docker volumes
- Incremental builds are supported

### Resource Management
- Development container uses ~2GB RAM
- Full build requires ~5GB disk space
- ARM toolchain cache saves download time

### Network Access
- Host networking for USB device access
- Port forwarding for web services
- Local network access for embedded system communication

## Security Considerations

### Container Security
- Containers run with user privileges (non-root)
- USB access requires privileged mode for hardware programming
- Source code is mounted read-write for development

### Best Practices
- Don't store sensitive data in containers
- Use specific image tags for production
- Regular security updates of base images

## Additional Resources

- [Docker Documentation](https://docs.docker.com/)
- [Docker Compose Reference](https://docs.docker.com/compose/)
- [STM32 Development with Docker](https://www.st.com/en/development-tools.html)
- [ARM GCC Toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm)

## Contributing

When contributing Docker improvements:

1. Test changes with `./docker-scripts/setup.sh`
2. Verify all services work correctly
3. Update documentation
4. Consider build time and image size impact

For more details, see [CONTRIBUTING.md](../CONTRIBUTING.md).