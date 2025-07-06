#!/usr/bin/env python3
"""
UI Launcher - Choose between Basic and Improved UI versions
"""

import sys
import os
from pathlib import Path

def show_menu():
    """Show UI selection menu"""
    print("STM32N6 Object Detection - UI Launcher")
    print("=" * 40)
    print()
    print("Available UI versions:")
    print("1. Basic UI (Fixed) - Original with stability improvements")
    print("2. Improved UI - Enhanced with buffering and performance optimizations")
    print("3. Simple Test UI - Minimal test version")
    print("4. Exit")
    print()
    
    while True:
        try:
            choice = input("Select UI version (1-4): ").strip()
            
            if choice == "1":
                return "basic_ui.py"
            elif choice == "2":
                return "improved_basic_ui.py"
            elif choice == "3":
                return "simple_ui_test.py"
            elif choice == "4":
                return None
            else:
                print("Invalid choice. Please enter 1, 2, 3, or 4.")
                
        except (KeyboardInterrupt, EOFError):
            return None

def main():
    """Main launcher function"""
    # Change to the python_tools directory
    script_dir = Path(__file__).parent
    os.chdir(script_dir)
    
    ui_file = show_menu()
    
    if ui_file is None:
        print("Exiting...")
        return 0
    
    print(f"\nLaunching {ui_file}...")
    print("=" * 40)
    
    try:
        # Import and run the selected UI
        if ui_file == "basic_ui.py":
            from basic_ui import main as ui_main
        elif ui_file == "improved_basic_ui.py":
            from improved_basic_ui import main as ui_main
        elif ui_file == "simple_ui_test.py":
            from simple_ui_test import main as ui_main
        
        ui_main()
        
    except ImportError as e:
        print(f"Failed to import {ui_file}: {e}")
        print("Make sure all required packages are installed:")
        print("  pip install -r requirements_minimal.txt")
        return 1
    except Exception as e:
        print(f"Error running {ui_file}: {e}")
        return 1
    
    return 0

if __name__ == "__main__":
    sys.exit(main())