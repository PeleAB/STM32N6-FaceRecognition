@echo off
REM Build script for creating Windows executable of STM32N6 Object Detection UI
REM Run this on Windows with Python and PyInstaller installed

echo STM32N6 Object Detection UI - Windows Build
echo =============================================

REM Check if PyInstaller is installed
python -c "import PyInstaller" 2>nul
if errorlevel 1 (
    echo Installing PyInstaller...
    pip install pyinstaller
)

REM Clean previous builds
if exist dist rmdir /s /q dist
if exist build rmdir /s /q build

REM Create the executable
echo Building executable...
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

if exist dist\STM32N6_ObjectDetection_UI.exe (
    echo.
    echo ✓ Build completed successfully!
    echo ✓ Executable location: dist\STM32N6_ObjectDetection_UI.exe
    echo.
    echo Creating portable package...
    
    REM Create portable directory
    if exist STM32N6_ObjectDetection_UI_Portable rmdir /s /q STM32N6_ObjectDetection_UI_Portable
    mkdir STM32N6_ObjectDetection_UI_Portable
    
    REM Copy executable
    copy dist\STM32N6_ObjectDetection_UI.exe STM32N6_ObjectDetection_UI_Portable\
    copy requirements_minimal.txt STM32N6_ObjectDetection_UI_Portable\
    
    REM Create README
    echo # STM32N6 Object Detection UI - Portable Version > STM32N6_ObjectDetection_UI_Portable\README.txt
    echo. >> STM32N6_ObjectDetection_UI_Portable\README.txt
    echo ## Quick Start >> STM32N6_ObjectDetection_UI_Portable\README.txt
    echo 1. Connect your STM32N6570-DK board to your computer >> STM32N6_ObjectDetection_UI_Portable\README.txt
    echo 2. Run STM32N6_ObjectDetection_UI.exe >> STM32N6_ObjectDetection_UI_Portable\README.txt
    echo 3. Select COM port and baud rate >> STM32N6_ObjectDetection_UI_Portable\README.txt
    echo 4. Click Connect to start >> STM32N6_ObjectDetection_UI_Portable\README.txt
    
    echo ✓ Portable package created: STM32N6_ObjectDetection_UI_Portable\
    echo.
    echo Ready for distribution!
) else (
    echo ✗ Build failed!
    echo Check the output above for errors.
)

pause