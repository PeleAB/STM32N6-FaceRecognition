#!/usr/bin/env python3
"""
Enhanced UI Launcher with dependency checking and setup
"""

import sys
import subprocess
import importlib
from pathlib import Path

def check_dependencies():
    """Check if all required dependencies are installed"""
    required_packages = [
        'numpy', 'cv2', 'serial', 'PySide6', 'onnxruntime'
    ]
    
    missing_packages = []
    for package in required_packages:
        try:
            if package == 'cv2':
                importlib.import_module('cv2')
            else:
                importlib.import_module(package)
        except ImportError:
            missing_packages.append(package)
    
    return missing_packages

def install_requirements():
    """Install requirements if missing"""
    requirements_file = Path(__file__).parent / "requirements.txt"
    if requirements_file.exists():
        print("Installing required packages...")
        subprocess.check_call([
            sys.executable, "-m", "pip", "install", "-r", str(requirements_file)
        ])
        return True
    return False

def main():
    """Main launcher function"""
    print("STM32N6 Object Detection - Enhanced UI Launcher")
    print("=" * 50)
    
    # Check dependencies
    missing = check_dependencies()
    if missing:
        print(f"Missing packages: {', '.join(missing)}")
        try:
            install_requirements()
            print("Dependencies installed successfully!")
        except Exception as e:
            print(f"Failed to install dependencies: {e}")
            print("Please install manually using:")
            print(f"pip install -r {Path(__file__).parent / 'requirements.txt'}")
            return 1
    
    # Launch enhanced UI
    try:
        print("Launching Enhanced UI...")
        from enhanced_ui import main as ui_main
        ui_main()
    except ImportError as e:
        print(f"Failed to import enhanced UI: {e}")
        print("Falling back to legacy UI...")
        try:
            from pc_uart_client import main as legacy_main
            legacy_main()
        except ImportError:
            print("No UI available. Please check installation.")
            return 1
    except Exception as e:
        print(f"UI error: {e}")
        return 1
    
    return 0

if __name__ == "__main__":
    sys.exit(main())