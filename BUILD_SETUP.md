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

### 3. STM32 Tools Configuration

Configure your STM32 development tools:

1. Edit `stm32_tools_config.json` and set the correct paths for:
   - STM32CubeIDE installation
   - STM32EdgeAI (stedgeai) tool
   - STM32CubeProgrammer 
   - STM32 Signing Tool
   - ARM GCC toolchain

2. Use the provided scripts for model compilation and firmware deployment:
   - `./scripts/compile_model.sh` - Convert ONNX/TFLite models
   - `./scripts/sign_binary.sh` - Sign application binaries
   - `./scripts/flash_firmware.sh` - Flash firmware to board

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