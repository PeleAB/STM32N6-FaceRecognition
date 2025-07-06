#!/usr/bin/env python3
"""
UI Launcher - Start the working UI for STM32N6 Object Detection
"""

import sys
import os
from pathlib import Path

def main():
    print("STM32N6 Object Detection - UI Launcher")
    print("=" * 40)
    
    # Change to the python_tools directory
    script_dir = Path(__file__).parent
    os.chdir(script_dir)
    
    # Try to import and run the basic UI
    try:
        print("Starting Basic UI (minimal dependencies)...")
        from basic_ui import main as basic_main
        basic_main()
    except Exception as e:
        print(f"Failed to start Basic UI: {e}")
        
        # Fallback to simple test
        try:
            print("Falling back to Simple UI Test...")
            from simple_ui_test import main as test_main
            test_main()
        except Exception as e2:
            print(f"All UI options failed: {e2}")
            print("\nTroubleshooting:")
            print("1. Make sure minimal requirements are installed:")
            print("   pip install -r requirements_minimal.txt")
            print("2. Check if you have display access for GUI applications")
            return 1
    
    return 0

if __name__ == "__main__":
    sys.exit(main())