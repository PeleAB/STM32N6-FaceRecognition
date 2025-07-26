#!/usr/bin/env python3
"""
Test script to check serial port detection and permissions
"""

import os
import serial
from serial.tools import list_ports

def test_port_detection():
    """Test serial port detection"""
    print("=== Serial Port Detection Test ===")
    
    # List all COM ports
    print("\n1. All detected COM ports:")
    try:
        ports = list_ports.comports()
        for port in ports:
            print(f"   {port.device} - {port.description}")
    except Exception as e:
        print(f"   Error: {e}")
    
    # Check specific devices
    print("\n2. Checking common STM32 ports:")
    common_ports = ["/dev/ttyACM0", "/dev/ttyACM1", "/dev/ttyUSB0", "/dev/ttyUSB1"]
    
    for port_name in common_ports:
        if os.path.exists(port_name):
            print(f"   {port_name}: EXISTS")
            
            # Check permissions
            try:
                stat_info = os.stat(port_name)
                print(f"      Permissions: {oct(stat_info.st_mode)[-3:]}")
                print(f"      Owner: {stat_info.st_uid}, Group: {stat_info.st_gid}")
            except Exception as e:
                print(f"      Permission check failed: {e}")
            
            # Try to open
            try:
                test_serial = serial.Serial(port_name, 9600, timeout=0.1)
                test_serial.close()
                print(f"      Access: OK")
            except PermissionError as e:
                print(f"      Access: DENIED - {e}")
            except Exception as e:
                print(f"      Access: ERROR - {e}")
        else:
            print(f"   {port_name}: NOT FOUND")
    
    # Check user groups
    print(f"\n3. Current user groups:")
    try:
        import grp
        import pwd
        username = pwd.getpwuid(os.getuid()).pw_name
        groups = [g.gr_name for g in grp.getgrall() if username in g.gr_mem]
        primary_group = grp.getgrgid(pwd.getpwuid(os.getuid()).pw_gid).gr_name
        all_groups = [primary_group] + groups
        print(f"   User: {username}")
        print(f"   Groups: {', '.join(all_groups)}")
        print(f"   In dialout group: {'dialout' in all_groups}")
    except Exception as e:
        print(f"   Error checking groups: {e}")

if __name__ == "__main__":
    test_port_detection()