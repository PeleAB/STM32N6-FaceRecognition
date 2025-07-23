# Build Setup Instructions

## After Cloning the Repository

After cloning this repository, you need to clean and rebuild the STM32CubeIDE project due to build artifacts that may have been included.

### 1. Clean Build Artifacts

```bash
# Remove any remaining build files
rm -rf STM32CubeIDE/Debug/
rm -rf STM32CubeIDE/Release/
rm -rf build/
```

### 2. STM32CubeIDE Setup

1. Open STM32CubeIDE
2. Import the project:
   - File → Import → General → Existing Projects into Workspace
   - Select the root directory of this repository
   - Check "Copy projects into workspace" (recommended)
3. Clean and rebuild:
   - Right-click project → Clean Project
   - Right-click project → Build Project

### 3. Docker Build Setup

If using Docker development environment:

```bash
# Run the setup script
./docker-scripts/setup.sh

# Start development environment
./docker-scripts/dev.sh

# Build firmware
./docker-scripts/build.sh
```

### 4. Makefile Build

For command-line builds:

```bash
# Clean previous build
make clean

# Build project
make -j$(nproc)
```

## Build Artifacts

The following files are generated during build and excluded from git:
- `*.d` - Dependency files
- `*.o` - Object files
- `*.su` - Stack usage files
- `*.cyclo` - Cyclomatic complexity files
- `*.elf` - Executable files
- `*.bin`, `*.hex` - Binary output files
- `*.list` - Assembly listing files
- `*.map` - Memory map files

Keep only the essential binaries in the `Binary/` directory for deployment.