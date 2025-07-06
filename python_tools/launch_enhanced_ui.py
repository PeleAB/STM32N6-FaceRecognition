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
        ('numpy', 'numpy'),
        ('cv2', 'opencv-python'), 
        ('serial', 'pyserial'),
        ('PySide6', 'PySide6'),
        ('onnxruntime', 'onnxruntime')
    ]
    
    missing_packages = []
    for module_name, package_name in required_packages:
        try:
            importlib.import_module(module_name)
            print(f"✓ {package_name} is available")
        except ImportError:
            missing_packages.append(package_name)
            print(f"✗ {package_name} is missing")
    
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
    
    # Test basic UI first
    try:
        print("Testing basic UI functionality...")
        from simple_ui_test import main as test_main
        
        # Ask user if they want to run the test
        response = input("Run simple UI test first? (y/n): ").lower()
        if response == 'y':
            test_main()
            return 0
            
    except Exception as e:
        print(f"Basic UI test failed: {e}")
    
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
            print("Legacy UI also not available.")
            print("Running simple UI test instead...")
            try:
                from simple_ui_test import main as test_main
                test_main()
            except Exception as test_e:
                print(f"All UI options failed: {test_e}")
                return 1
    except Exception as e:
        print(f"UI error: {e}")
        print("If this is a QAction error, the fixes should resolve it.")
        return 1
    
    return 0

if __name__ == "__main__":
    sys.exit(main())