#!/usr/bin/env python3
"""
Build script for creating Windows executable of STM32N6 Object Detection UI
"""

import os
import sys
import subprocess
import shutil
from pathlib import Path

def create_spec_file():
    """Create PyInstaller spec file for the UI"""
    
    spec_content = '''# -*- mode: python ; coding: utf-8 -*-

block_cipher = None

a = Analysis(
    ['basic_ui.py'],
    pathex=[],
    binaries=[],
    datas=[
        ('requirements_minimal.txt', '.'),
        ('*.py', '.'),
    ],
    hiddenimports=[
        'PySide6.QtCore',
        'PySide6.QtGui', 
        'PySide6.QtWidgets',
        'cv2',
        'numpy',
        'serial',
        'serial.tools',
        'serial.tools.list_ports',
    ],
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=[
        'tensorflow',
        'pandas', 
        'matplotlib',
        'plotly',
        'scikit-learn',
        'onnxruntime',
    ],
    win_no_prefer_redirects=False,
    win_private_assemblies=False,
    cipher=block_cipher,
    noarchive=False,
)

pyz = PYZ(a.pure, a.zipped_data, cipher=block_cipher)

exe = EXE(
    pyz,
    a.scripts,
    a.binaries,
    a.zipfiles,
    a.datas,
    [],
    name='STM32N6_ObjectDetection_UI',
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=True,
    upx_exclude=[],
    runtime_tmpdir=None,
    console=False,  # Set to True if you want console window
    disable_windowed_traceback=False,
    argv_emulation=False,
    target_arch=None,
    codesign_identity=None,
    entitlements_file=None,
    icon=None,  # Add icon file path here if you have one
    version='version_info.txt'  # Add version info if available
)
'''
    
    with open('basic_ui.spec', 'w') as f:
        f.write(spec_content)
    
    print("✓ Created PyInstaller spec file: basic_ui.spec")

def create_version_info():
    """Create version information file for the executable"""
    
    version_content = '''# UTF-8
VSVersionInfo(
  ffi=FixedFileInfo(
    filevers=(1, 0, 0, 0),
    prodvers=(1, 0, 0, 0),
    mask=0x3f,
    flags=0x0,
    OS=0x40004,
    fileType=0x1,
    subtype=0x0,
    date=(0, 0)
  ),
  kids=[
    StringFileInfo(
      [
      StringTable(
        u'040904B0',
        [StringStruct(u'CompanyName', u'STM32N6 Object Detection Project'),
        StringStruct(u'FileDescription', u'STM32N6 Object Detection UI'),
        StringStruct(u'FileVersion', u'1.0.0.0'),
        StringStruct(u'InternalName', u'STM32N6_ObjectDetection_UI'),
        StringStruct(u'LegalCopyright', u'© 2024 STM32N6 Object Detection Project'),
        StringStruct(u'OriginalFilename', u'STM32N6_ObjectDetection_UI.exe'),
        StringStruct(u'ProductName', u'STM32N6 Object Detection UI'),
        StringStruct(u'ProductVersion', u'1.0.0.0')])
      ]), 
    VarFileInfo([VarStruct(u'Translation', [1033, 1200])])
  ]
)
'''
    
    with open('version_info.txt', 'w') as f:
        f.write(version_content)
    
    print("✓ Created version info file: version_info.txt")

def build_executable():
    """Build the executable using PyInstaller"""
    
    print("Building executable...")
    
    # Clean previous builds
    if os.path.exists('dist'):
        shutil.rmtree('dist')
    if os.path.exists('build'):
        shutil.rmtree('build')
    
    # Run PyInstaller
    cmd = [sys.executable, '-m', 'PyInstaller', '--clean', 'basic_ui.spec']
    
    result = subprocess.run(cmd, capture_output=True, text=True)
    
    if result.returncode == 0:
        print("✓ Executable built successfully!")
        print(f"✓ Output location: {os.path.abspath('dist')}")
        
        # List contents of dist directory
        dist_path = Path('dist')
        if dist_path.exists():
            print("\\nGenerated files:")
            for item in dist_path.iterdir():
                print(f"  - {item.name}")
    else:
        print("✗ Build failed!")
        print("STDOUT:", result.stdout)
        print("STDERR:", result.stderr)
        return False
    
    return True

def create_portable_package():
    """Create a portable package with the executable and dependencies"""
    
    package_dir = Path('STM32N6_ObjectDetection_UI_Portable')
    
    if package_dir.exists():
        shutil.rmtree(package_dir)
    
    package_dir.mkdir()
    
    # Copy executable
    exe_path = Path('dist/STM32N6_ObjectDetection_UI.exe')
    if exe_path.exists():
        shutil.copy2(exe_path, package_dir)
        print(f"✓ Copied executable to {package_dir}")
    
    # Copy requirements file for reference
    req_file = Path('requirements_minimal.txt')
    if req_file.exists():
        shutil.copy2(req_file, package_dir)
    
    # Create README for the portable package
    readme_content = '''# STM32N6 Object Detection UI - Portable Version

## Quick Start

1. **Connect your STM32N6570-DK board** to your computer via USB
2. **Run the application** by double-clicking `STM32N6_ObjectDetection_UI.exe`
3. **Select the correct COM port** from the dropdown
4. **Choose the baud rate** (default: 7372800)
5. **Click Connect** to start receiving data

## Features

- **Real-time image display** from STM32N6 camera
- **Serial communication** with embedded system
- **Performance statistics** (FPS, frame count, uptime)
- **Modern dark theme** interface
- **Minimal dependencies** - no TensorFlow required

## System Requirements

- **Windows 10/11** (64-bit)
- **Available USB/Serial port** for STM32N6 connection
- **No additional software installation required**

## Troubleshooting

### No COM ports detected
- Ensure STM32N6570-DK is connected via USB
- Install STM32 Virtual COM Port drivers if needed
- Check Windows Device Manager for COM ports

### Connection fails
- Verify correct COM port selection
- Try different baud rates (921600, 1843200, 3686400, 7372800)
- Check cable connections
- Ensure embedded firmware is running

### No image display
- Verify embedded system is streaming data
- Check serial communication logs in the application
- Ensure correct protocol compatibility

## Support

For issues and updates, visit the project repository.

Generated with Claude Code - STM32N6 Object Detection Project
'''
    
    with open(package_dir / 'README.txt', 'w') as f:
        f.write(readme_content)
    
    print(f"✓ Created portable package: {package_dir}")
    print(f"✓ Package size: {get_folder_size(package_dir):.2f} MB")

def get_folder_size(folder_path):
    """Calculate folder size in MB"""
    total_size = 0
    for dirpath, dirnames, filenames in os.walk(folder_path):
        for filename in filenames:
            filepath = os.path.join(dirpath, filename)
            total_size += os.path.getsize(filepath)
    return total_size / (1024 * 1024)

def main():
    """Main build process"""
    print("STM32N6 Object Detection UI - Executable Builder")
    print("=" * 50)
    
    # Change to python_tools directory
    script_dir = Path(__file__).parent
    os.chdir(script_dir)
    
    try:
        # Step 1: Create spec file
        create_spec_file()
        
        # Step 2: Create version info
        create_version_info()
        
        # Step 3: Build executable
        if not build_executable():
            return 1
        
        # Step 4: Create portable package
        create_portable_package()
        
        print("\\n" + "=" * 50)
        print("✓ Build completed successfully!")
        print("\\nNext steps:")
        print("1. Test the executable on Windows")
        print("2. Distribute the portable package")
        print("3. Share with users who need the UI")
        
        return 0
        
    except Exception as e:
        print(f"\\n✗ Build failed with error: {e}")
        return 1

if __name__ == "__main__":
    sys.exit(main())