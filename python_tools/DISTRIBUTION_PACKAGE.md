# STM32N6 Object Detection UI - Distribution Package

## Package Contents

This package contains everything needed to build and distribute the STM32N6 Object Detection UI as a Windows executable.

### Files Included

#### For Windows Users (End Users)
- `build_windows_exe.bat` - **One-click build script for Windows**
- `basic_ui.py` - Main UI application source
- `requirements_minimal.txt` - Required Python packages
- `BUILD_WINDOWS_INSTRUCTIONS.md` - Detailed build instructions

#### For Developers
- `build_exe.py` - Advanced Python build script  
- `basic_ui.spec` - PyInstaller specification file
- `version_info.txt` - Windows executable version information
- `run_ui.py` - Alternative launcher script

#### Current Build Results (Linux)
- `STM32N6_ObjectDetection_UI_Portable/` - Contains Linux executable and docs
- `dist/` - PyInstaller output directory

## Quick Start for Windows Users

**To create a Windows executable:**

1. **Download this entire folder** to a Windows machine with Python installed
2. **Double-click** `build_windows_exe.bat`
3. **Wait** for the build to complete (2-5 minutes)
4. **Find your executable** in `dist/STM32N6_ObjectDetection_UI.exe`

**To use the UI:**

1. **Connect** your STM32N6570-DK board to USB
2. **Run** the executable
3. **Select** the correct COM port and baud rate
4. **Click Connect** to start receiving data

## Features

### UI Capabilities
- **Real-time image display** from STM32N6 camera
- **Serial communication** with embedded system  
- **Performance monitoring** (FPS, frame count, uptime)
- **Modern dark theme** interface
- **Connection management** with auto-reconnect

### Technical Benefits
- **Minimal dependencies** - No TensorFlow required
- **Standalone executable** - No Python installation needed for end users
- **Cross-platform source** - Same code works on Windows and Linux
- **Robust protocol** - Compatible with enhanced and basic streaming

### Deployment Advantages
- **Single file distribution** - Just share the .exe
- **No installation required** - Run directly
- **Portable** - Works from any folder/USB drive
- **User-friendly** - Simple GUI interface

## System Requirements

### For Building (Developer)
- **Windows 10/11** with Python 3.8+
- **Python packages**: PySide6, OpenCV, NumPy, PySerial, PyInstaller
- **Disk space**: ~2 GB for build environment

### For Running (End User)  
- **Windows 10/11** (64-bit)
- **Available USB/COM port** for STM32N6
- **Disk space**: ~150 MB for executable
- **No additional software** required

## Build Outputs

### Expected File Sizes
- **Source code**: ~50 KB
- **Executable**: ~130-150 MB (includes Python runtime)
- **Portable package**: ~150 MB total

### Distribution Options
1. **Single executable**: Just the .exe file
2. **Portable package**: Folder with .exe + documentation  
3. **Installer**: Could be created with NSIS/InnoSetup (future enhancement)

## Troubleshooting

### Common Build Issues
- **Python not found**: Install Python from python.org
- **PyInstaller errors**: Update to latest version
- **Missing modules**: Install from requirements_minimal.txt

### Common Runtime Issues
- **No COM ports**: Install STM32 Virtual COM Port drivers
- **Antivirus warnings**: Executable is unsigned (normal for indie dev)
- **Slow startup**: Normal for bundled Python apps (3-5 seconds)

## Support & Updates

### Getting Help
- Check `BUILD_WINDOWS_INSTRUCTIONS.md` for detailed steps
- Review console output for specific error messages
- Ensure all prerequisites are installed

### Future Enhancements
- **Code signing** for trusted execution
- **Auto-updater** functionality  
- **Installer package** creation
- **macOS support** (if needed)

## Legal Notes

- **Open source** - Modify and distribute freely
- **No warranty** - Use at your own risk
- **STM32 trademarks** belong to STMicroelectronics
- **Generated with Claude Code** - AI-assisted development

---

**Ready to build? Run `build_windows_exe.bat` on Windows!**