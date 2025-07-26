#!/usr/bin/env python3
"""
Launcher script for STM32N6 Face Recognition UI
"""

import sys
import subprocess
import pkg_resources
from pathlib import Path

def check_requirements():
    """Check if required packages are installed"""
    requirements_file = Path(__file__).parent / "requirements.txt"
    
    if not requirements_file.exists():
        print("Requirements file not found!")
        return False
    
    try:
        with open(requirements_file, 'r') as f:
            requirements = []
            for line in f:
                line = line.strip()
                if line and not line.startswith('#'):
                    # Extract package name (before >= or ==)
                    package = line.split('>=')[0].split('==')[0]
                    requirements.append(package)
        
        missing = []
        for requirement in requirements:
            try:
                pkg_resources.get_distribution(requirement)
            except pkg_resources.DistributionNotFound:
                missing.append(requirement)
        
        if missing:
            print("Missing required packages:")
            for pkg in missing:
                print(f"  - {pkg}")
            print(f"\\nInstall with: pip install -r {requirements_file}")
            return False
        
        return True
        
    except Exception as e:
        print(f"Error checking requirements: {e}")
        return False

def main():
    """Main launcher"""
    print("STM32N6 Face Recognition UI Launcher")
    print("=" * 40)
    
    # Check if we're in a virtual environment
    venv_path = Path(__file__).parent / "venv"
    in_venv = hasattr(sys, 'real_prefix') or (hasattr(sys, 'base_prefix') and sys.base_prefix != sys.prefix)
    
    if not in_venv and venv_path.exists():
        print("Virtual environment detected but not activated.")
        print("Please run:")
        print("  source venv/bin/activate && python run_ui.py")
        print("Or use:")
        print("  ./activate_and_run.sh")
        sys.exit(1)
    
    # Check requirements
    if not check_requirements():
        print("\\nMissing requirements detected.")
        if venv_path.exists():
            print("Try: source venv/bin/activate && pip install -r requirements.txt")
        else:
            print("Try: python3 -m venv venv && source venv/bin/activate && pip install -r requirements.txt")
        sys.exit(1)
    
    print("All requirements satisfied!")
    print("Starting Face Recognition UI...")
    
    try:
        # Import and run the UI
        from face_recognition_ui import main as run_ui
        run_ui()
        
    except ImportError as e:
        print(f"Import error: {e}")
        print("Make sure all required packages are installed in the virtual environment.")
        sys.exit(1)
    except KeyboardInterrupt:
        print("\\nApplication interrupted by user.")
    except Exception as e:
        print(f"Error running UI: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()