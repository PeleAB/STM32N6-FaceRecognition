# STM32N6 Object Detection UI - Distribution Package

## FOR WINDOWS USERS: How to Create the .exe File

**This package contains the source code. To create the Windows executable:**

1. **Install Python 3.8+** from https://python.org (if not already installed)
2. **Open Command Prompt** in this folder
3. **Run the build script:**
   ```
   build_windows_exe.bat
   ```
4. **Wait 2-5 minutes** for the build to complete
5. **Find your executable** in the `dist` folder: `STM32N6_ObjectDetection_UI.exe`

## FOR LINUX USERS: Build or Run with Python

- **Build executable:** `python3 build_exe.py`
- **Or run with Python:** `python3 basic_ui.py`

## Using the Application

### Quick Start
1. **Connect STM32N6570-DK board** to your computer via USB
2. **Run the executable** (Windows: .exe, Linux: binary file)
3. **Select COM port** from dropdown (e.g., COM3, /dev/ttyUSB0)
4. **Choose baud rate** (try 7372800 first, then 921600)
5. **Click Connect** to start receiving data

### Features
- **Real-time image display** from STM32N6 camera
- **Serial communication** with embedded system
- **Performance statistics** (FPS, frame count, uptime)
- **Modern dark theme** interface
- **Connection auto-reconnect** capability
- **Minimal dependencies** - no TensorFlow required

## System Requirements

### Windows
- **Windows 10/11** (64-bit)
- **Python 3.8+** (for building only)
- **USB/Serial port** for STM32N6 connection

### Linux  
- **Ubuntu 20.04+** or compatible distribution
- **Python 3.8+** and libraries (for source version)
- **USB/Serial port** for STM32N6 connection

## Files in This Package

- `build_windows_exe.bat` - **Click this to build Windows .exe**
- `basic_ui.py` - Main application source code
- `requirements_minimal.txt` - Required Python packages
- Linux executable can be built with `python3 build_exe.py`
- `BUILD_WINDOWS_INSTRUCTIONS.md` - Detailed build instructions
- `DISTRIBUTION_PACKAGE.md` - Complete documentation

## Troubleshooting

### Build Issues (Windows)
- **Python not found**: Install from python.org and restart Command Prompt
- **pip not found**: Reinstall Python with "Add to PATH" option checked
- **PyInstaller errors**: Run `pip install --upgrade pyinstaller`

### Runtime Issues
- **No COM ports**: Install STM32 Virtual COM Port drivers
- **Connection fails**: Try different baud rates or check USB cable
- **Antivirus warning**: Executable is unsigned (safe to run)
- **Slow startup**: Normal for bundled apps (3-5 seconds)

### Serial Communication
- **Windows**: COM ports (COM1, COM2, etc.)
- **Linux**: /dev/ttyUSB0, /dev/ttyACM0, etc.
- **Baud rates**: Try 7372800, 3686400, 1843200, 921600

## Support & Development

- **Source code**: All files included for modification
- **Updates**: Check project repository for latest version
- **Issues**: Report bugs with detailed error messages
- **License**: Open source - modify and distribute freely

---

**Ready to build? Run build_windows_exe.bat on Windows!**

Generated with Claude Code - STM32N6 Object Detection Project