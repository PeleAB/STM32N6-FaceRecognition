# Building Windows Executable for STM32N6 Object Detection UI

## Prerequisites

**On Windows machine with Python 3.8+ installed:**

1. **Install Python dependencies:**
   ```cmd
   pip install -r requirements_minimal.txt
   pip install pyinstaller
   ```

2. **Verify the UI works:**
   ```cmd
   python basic_ui.py
   ```

## Option 1: Quick Build (Recommended)

**Run the provided batch file:**
```cmd
build_windows_exe.bat
```

This will:
- Install PyInstaller if needed
- Build a single-file executable
- Create a portable distribution package
- Include documentation

## Option 2: Manual Build

**Create the executable manually:**

```cmd
pyinstaller --onefile ^
    --windowed ^
    --name "STM32N6_ObjectDetection_UI" ^
    --add-data "requirements_minimal.txt;." ^
    --hidden-import "PySide6.QtCore" ^
    --hidden-import "PySide6.QtGui" ^
    --hidden-import "PySide6.QtWidgets" ^
    --hidden-import "cv2" ^
    --hidden-import "numpy" ^
    --hidden-import "serial" ^
    --hidden-import "serial.tools" ^
    --hidden-import "serial.tools.list_ports" ^
    --exclude-module "tensorflow" ^
    --exclude-module "pandas" ^
    --exclude-module "matplotlib" ^
    --exclude-module "plotly" ^
    --exclude-module "scikit-learn" ^
    --exclude-module "onnxruntime" ^
    basic_ui.py
```

## Option 3: Advanced Build with Spec File

**Use the Python build script:**
```cmd
python build_exe.py
```

This provides more control and creates a complete package.

## Expected Output

After successful build:
- **Executable**: `dist/STM32N6_ObjectDetection_UI.exe` (~130-150 MB)
- **Portable Package**: `STM32N6_ObjectDetection_UI_Portable/` folder

## Build Parameters Explained

- `--onefile`: Creates a single executable file (easier distribution)
- `--windowed`: No console window (GUI app)
- `--add-data`: Include additional files in the executable
- `--hidden-import`: Ensure required modules are included
- `--exclude-module`: Remove unnecessary large dependencies

## Troubleshooting

### PyInstaller Not Found
```cmd
pip install pyinstaller
```

### Missing Dependencies
```cmd
pip install PySide6 opencv-python numpy pyserial
```

### Large Executable Size
- Normal for bundled Python applications (130-150 MB)
- Includes Python runtime, PySide6, OpenCV, and NumPy
- Can be reduced with `--exclude-module` for unused libraries

### Import Errors
- Add missing modules with `--hidden-import MODULE_NAME`
- Check the build console output for missing dependencies

## Distribution

The generated executable is **standalone** and can be distributed to users without requiring Python installation.

**System Requirements:**
- Windows 10/11 (64-bit)
- No additional software needed
- USB port for STM32N6 connection

**Portable Package Contents:**
- `STM32N6_ObjectDetection_UI.exe` - Main application
- `README.txt` - User instructions  
- `requirements_minimal.txt` - Reference for dependencies

## Testing

Before distribution:
1. Test the executable on a clean Windows machine
2. Verify serial port detection works
3. Test UI functionality without development environment
4. Check antivirus doesn't flag the executable

## Performance Notes

- **Startup time**: 3-5 seconds (normal for bundled app)
- **Memory usage**: ~100-200 MB (includes GUI framework)
- **Runtime performance**: Same as running Python script directly

## Security Notes

- Executable is **unsigned** (may trigger Windows SmartScreen)
- Users can bypass with "More info" → "Run anyway"
- For production: Consider code signing certificate

---

**Generated with Claude Code - STM32N6 Object Detection Project**